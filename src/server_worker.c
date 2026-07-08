/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "server.h"
#include "server_utils.h"
#include "logger.h"
#include "http_parser.h"
#include "http_router.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * Worker permanent du pool. Attend et consomme les sockets de la file d'attente FIFO.
 */
void *lith_pool_worker(void *arg) {
    ThreadPool_t *pool = (ThreadPool_t *)arg;

    // Allocation unique du tampon de lecture pour toute la durée de vie du thread
    char *full_buffer = malloc(BUFFER_SIZE * 4);
    if (!full_buffer) {
        lith_log(LOG_ERROR, "Memory allocation failed for pool worker thread");
        return NULL;
    }

    while (1) {
        socket_t client_socket;

        // ─── SECTION CRITIQUE : EXTRACTION DE LA SOCKET ───
        pthread_mutex_lock(&(pool->mutex));
        while (pool->head == NULL && !pool->shutdown) {
            pthread_cond_wait(&(pool->cond), &(pool->mutex));
        }

        // Vérification de l'ordre d'arrêt global du serveur
        if (pool->shutdown) {
            pthread_mutex_unlock(&(pool->mutex));
            break;
        }

        // Pop (FIFO) depuis la file d'attente
        Node_t *temp = pool->head;
        client_socket = temp->socket;
        pool->head = pool->head->next;
        if (pool->head == NULL) {
            pool->tail = NULL;
        }
        pool->size--;
        free(temp);
        pthread_mutex_unlock(&(pool->mutex));

        // ─── INITIALISATION DU CONTEXTE DE REQUÊTE ───
        ExpandedClientContext *ectx = malloc(sizeof(ExpandedClientContext));
        if (!ectx) {
            lith_log(LOG_ERROR, "Failed to allocate memory for request context");
            lith_close_socket(client_socket);
            continue;
        }
        memset(ectx, 0, sizeof(ExpandedClientContext));
        ectx->base_ctx.client_socket = client_socket;
        
        // CORRIGÉ : Extraction thread-safe sans variable statique globale
        strncpy(ectx->public_dir, pool->public_dir, sizeof(ectx->public_dir) - 1);

        // ─── BOUCLE PERSISTANTE HTTP/1.1 KEEP-ALIVE ───
        bool connection_keep_active = true;
        int request_count = 0;
        const int MAX_KEEP_ALIVE_REQUESTS = 100;

        while (connection_keep_active && request_count < MAX_KEEP_ALIVE_REQUESTS) {
            memset(full_buffer, 0, BUFFER_SIZE * 4);
            int total_received = recv(ectx->base_ctx.client_socket, full_buffer, (BUFFER_SIZE * 4) - 1, 0);

            if (total_received > 0) {
                HttpRequest req;
                if (parse_http_request(full_buffer, &req) != 0) {
                    lith_log(LOG_WARN, "400 - Bad Request: parsing failed");
                    send_http_error(ectx->base_ctx.client_socket, 400, "Bad Request", "The server could not understand the request due to malformed syntax.");
                    break;
                }

                // --- GESTION EN AMONT DU CORPS POST (Sécurité Anti-Slowloris) ---
                if (req.method == METHOD_POST && req.content_length > 0) {
                    long already_received_body = 0;
                    bool post_timeout_triggered = false;
                    
                    if (req.body_start) {
                        already_received_body = total_received - (req.body_start - full_buffer);
                    }

                    while (already_received_body < req.content_length) {
                        long remaining = req.content_length - already_received_body;
                        if (total_received >= (BUFFER_SIZE * 4) - 1) {
                            lith_log(LOG_WARN, "413 - Payload Too Large: Client bytes exceed allocated space");
                            break;
                        }
                        
                        int n = recv(ectx->base_ctx.client_socket, full_buffer + total_received, remaining, 0);
                        if (n <= 0) {
                            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                                lith_log(LOG_WARN, "Network receive timeout triggered during POST body streaming");
                                post_timeout_triggered = true;
                            }
                            break;
                        }
                        total_received += n;
                        already_received_body += n;
                    }

                    if (post_timeout_triggered) {
                        send_http_error(ectx->base_ctx.client_socket, 408, "Request Timeout", "The server timed out waiting for the full POST body payload.");
                        break; // Brise la persistance pour fermer la socket immédiatement
                    }
                    
                    parse_http_request(full_buffer, &req);
                }

                // Routage de la requête applicative
                handle_http_route(ectx, &req, full_buffer, total_received);
                request_count++;
                connection_keep_active = req.keep_alive;

            } else {
                // Gestion des déconnexions et des timeouts de la fenêtre Keep-Alive
                if (total_received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    if (request_count == 0) {
                        lith_log(LOG_WARN, "408 - Request Timeout: Client failed to send data within the window");
                        send_http_error(ectx->base_ctx.client_socket, 408, "Request Timeout", "The server timed out waiting for the request.");
                    } else {
                        lith_log(LOG_INFO, "Keep-Alive window closed cleanly via network timeout");
                    }
                }
                connection_keep_active = false;
            }
        }

        // Libération des ressources de la tâche courante et scellage de la connexion
        lith_close_socket(ectx->base_ctx.client_socket);
        free(ectx);
    }

    free(full_buffer);
    return NULL;
}

/**
 * Initialise les structures du Thread Pool et lève les workers permanents
 */
void thread_pool_init(ThreadPool_t *pool, int num_threads, const char *public_dir) {
    if (!pool || num_threads <= 0) return;

    pool->head = NULL;
    pool->tail = NULL;
    pool->size = 0;
    pool->shutdown = false;
    
    // Sauvegarde de la configuration directement dans l'état interne du pool
    memset(pool->public_dir, 0, sizeof(pool->public_dir));
    if (public_dir) {
        strncpy(pool->public_dir, public_dir, sizeof(pool->public_dir) - 1);
    } else {
        strncpy(pool->public_dir, "public", sizeof(pool->public_dir) - 1);
    }

    pthread_mutex_init(&(pool->mutex), NULL);
    pthread_cond_init(&(pool->cond), NULL);

    // Instanciation des workers
    for (int i = 0; i < num_threads; i++) {
        pthread_t th;
        if (pthread_create(&th, NULL, lith_pool_worker, pool) == 0) {
            pthread_detach(th); // Recyclage automatique des threads par l'OS à leur sortie
        }
    }
    lith_log(LOG_INFO, "Thread Pool allocated with %d permanent architectural workers", num_threads);
}

/**
 * Enfile une nouvelle connexion cliente et réveille un thread disponible
 */
void thread_pool_push(ThreadPool_t *pool, socket_t socket) {
    if (!pool) return;

    pthread_mutex_lock(&(pool->mutex));
    
    // Si le pool est en cours d'extinction, on refuse la tâche
    if (pool->shutdown) {
        pthread_mutex_unlock(&(pool->mutex));
        lith_close_socket(socket);
        return;
    }

    Node_t *node = malloc(sizeof(Node_t));
    if (!node) {
        lith_log(LOG_ERROR, "Critical memory exhaustion: cannot enqueue client connection");
        pthread_mutex_unlock(&(pool->mutex));
        lith_close_socket(socket);
        return;
    }
    node->socket = socket;
    node->next = NULL;

    // Insertion en queue (FIFO)
    if (pool->tail == NULL) {
        pool->head = node;
        pool->tail = node;
    } else {
        pool->tail->next = node;
        pool->tail = node;
    }
    pool->size++;
    
    pthread_cond_signal(&(pool->cond)); // Signalement pour réveiller un worker en attente
    pthread_mutex_unlock(&(pool->mutex));
}

/**
 * Arrête proprement le Thread Pool et libère les requêtes en suspens
 */
void thread_pool_shutdown(ThreadPool_t *pool) {
    if (!pool) return;

    pthread_mutex_lock(&(pool->mutex));
    pool->shutdown = true;
    pthread_cond_broadcast(&(pool->cond)); // Réveil forcé de TOUS les workers pour qu'ils quittent
    pthread_mutex_unlock(&(pool->mutex));

    // Nettoyage de la file d'attente si des tâches n'ont pas été traitées
    pthread_mutex_lock(&(pool->mutex));
    Node_t *current = pool->head;
    while (current != NULL) {
        Node_t *next = current->next;
        lith_close_socket(current->socket);
        free(current);
        current = next;
    }
    pool->head = NULL;
    pool->tail = NULL;
    pool->size = 0;
    pthread_mutex_unlock(&(pool->mutex));
    
    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->cond));
    lith_log(LOG_INFO, "Thread Pool shutdown sequence completed cleanly");
}