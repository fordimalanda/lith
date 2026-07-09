/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "server.h"
#include "cache.h"
#include "logger.h"
#include "http_router.h"
#include "server_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

/**
 * Initialise le pool de workers et pré-charge le cache statique RAM (v1.0.9)
 */
void thread_pool_init(ThreadPool_t *pool, int num_threads, const char *public_dir) {
    pool->head = NULL;
    pool->tail = NULL;
    pool->size = 0;
    pool->shutdown = false;
    
    strncpy(pool->public_dir, public_dir, sizeof(pool->public_dir) - 1);
    pool->public_dir[sizeof(pool->public_dir) - 1] = '\0';

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);

    lith_cache_init(&pool->ram_cache);
    
    char path_buf[512];
    snprintf(path_buf, sizeof(path_buf), "%s/index.html", public_dir);
    lith_cache_add(&pool->ram_cache, path_buf, "text/html");
    
    snprintf(path_buf, sizeof(path_buf), "%s/style.css", public_dir);
    lith_cache_add(&pool->ram_cache, path_buf, "text/css");

    for (int i = 0; i < num_threads; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, lith_client_handler, (void *)pool) == 0) {
            pthread_detach(thread);
        }
    }
    
    lith_log(LOG_INFO, "Thread pool initialized successfully with %d active workers.", num_threads);
}

/**
 * Ajoute une connexion entrante dans la file d'attente FIFO (Thread-Safe)
 */
void thread_pool_push(ThreadPool_t *pool, socket_t socket) {
    Node_t *new_node = malloc(sizeof(Node_t));
    if (!new_node) {
        lith_log(LOG_ERROR, "Critical allocation failure for pool node");
        lith_close_socket(socket);
        return;
    }
    new_node->socket = socket;
    new_node->next = NULL;

    pthread_mutex_lock(&pool->mutex);
    
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutex);
        free(new_node);
        lith_close_socket(socket);
        return;
    }

    if (pool->tail == NULL) {
        pool->head = new_node;
        pool->tail = new_node;
    } else {
        pool->tail->next = new_node;
        pool->tail = new_node;
    }
    pool->size++;

    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
}

/**
 * Arrête proprement le pool de threads et détruit les verrous
 */
void thread_pool_shutdown(ThreadPool_t *pool) {
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);

    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    
    // Libération complète de la mémoire du cache RAM
    lith_cache_destroy(&pool->ram_cache);
}

/**
 * Fonction de démarrage des threads du pool (Consommateurs FIFO)
 */
void *lith_client_handler(void *arg) {
    ThreadPool_t *pool = (ThreadPool_t *)arg;

    while (true) {
        pthread_mutex_lock(&pool->mutex);

        while (pool->head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        Node_t *node = pool->head;
        pool->head = pool->head->next;
        if (pool->head == NULL) {
            pool->tail = NULL;
        }
        pool->size--;

        pthread_mutex_unlock(&pool->mutex);

        socket_t client_socket = node->socket;
        free(node);

        // Configuration d'un Timeout réseau de 3 secondes pour préserver les workers du pool
        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            lith_log(LOG_ERROR, "Failed to apply SO_RCVTIMEO context on client socket.");
        }

        ExpandedClientContext ectx;
        ectx.base_ctx.client_socket = client_socket;
        memset(&ectx.base_ctx.client_address, 0, sizeof(ectx.base_ctx.client_address));
        strncpy(ectx.public_dir, pool->public_dir, sizeof(ectx.public_dir) - 1);
        ectx.public_dir[sizeof(ectx.public_dir) - 1] = '\0';

        HttpRequest *req = malloc(sizeof(HttpRequest));
        if (req) {
            char buffer[BUFFER_SIZE];
            bool keep_running = true;

            // Boucle de traitement persistant Keep-Alive HTTP/1.1
            while (keep_running) {
                int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                
                if (received > 0) {
                    buffer[received] = '\0';
                    
                    if (parse_http_request(buffer, req) == 0) {
                        // Routage classique et exécution de la réponse
                        handle_http_route(&ectx, req, buffer, received);
                        
                        if (!req->keep_alive) {
                            keep_running = false;
                        }
                    } else {
                        // Requête invalide (400) -> on ferme la connexion de force (false)
                        send_http_error(client_socket, 400, "Bad Request", "Malformed HTTP request protocol.", false);
                        keep_running = false;
                    }
                } else {
                    // Déconnexion propre du client ou déclenchement du timeout SO_RCVTIMEO
                    keep_running = false;
                }
            }
            free(req);
        }

        lith_close_socket(client_socket);
    }

    return NULL;
}