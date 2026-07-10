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
#include <openssl/ssl.h>
#include <openssl/err.h>

// [vx.x.x] Gestion Multi-plateforme des Sockets et Timeouts SSL (Unix vs Windows)
#ifdef _WIN32
    #include <winsock2.h>
    #define SET_TIMEOUT_ERROR (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms)) < 0)
#else
    #include <sys/socket.h>
    #include <sys/time.h>
    #define SET_TIMEOUT_ERROR (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_tv, sizeof(timeout_tv)) < 0)
#endif

/**
 * Initialise le pool de workers et pré-charge le cache statique RAM
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
 * Extrait une connexion de la file FIFO (Thread-Safe & Bloquant)
 */
socket_t thread_pool_pop(ThreadPool_t *pool) {
    pthread_mutex_lock(&pool->mutex);

    while (pool->head == NULL && !pool->shutdown) {
        pthread_cond_wait(&pool->cond, &pool->mutex);
    }

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutex);
        return (socket_t)-1;
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
    return client_socket;
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
    
    lith_cache_destroy(&pool->ram_cache);
}

/**
 * Fonction de démarrage des threads du pool (Consommateurs FIFO TLS/HTTPS)
 */
void *lith_client_handler(void *arg) {
    ThreadPool_t *pool = (ThreadPool_t *)arg;
    SSL_CTX *ssl_ctx = lith_ssl_get_context();

    while (true) {
        socket_t client_socket = thread_pool_pop(pool);
        
        if (pool->shutdown || client_socket == (socket_t)-1) {
            break;
        }

        // Abstraction du Timeout (3000ms) avant Handshake cryptographique
#ifdef _WIN32
        int timeout_ms = 3000;
#else
        struct timeval timeout_tv;
        timeout_tv.tv_sec = 3;
        timeout_tv.tv_usec = 0;
#endif

        if (SET_TIMEOUT_ERROR) {
            lith_log(LOG_ERROR, "Failed to apply SO_RCVTIMEO context on client socket.");
        }

        // 1. Initialisation de la session sécurisée OpenSSL
        SSL *ssl = SSL_new(ssl_ctx);
        if (!ssl) {
            compress_log(LOG_ERROR, "SSL: Failed to allocate space for client session storage.");
            lith_close_socket(client_socket);
            continue;
        }

        SSL_set_fd(ssl, client_socket);

        // 2. Exécution de la poignée de main TLS (Handshake)
        if (SSL_accept(ssl) <= 0) {
            lith_log(LOG_WARN, "SSL: Cryptographic handshake failed. Dropping insecure connection request.");
            SSL_free(ssl);
            lith_close_socket(client_socket);
            continue;
        }

        // 3. Remplissage des contextes requis par l'infrastructure
        ExpandedClientContext ectx;
        ectx.base_ctx.client_socket = client_socket;
        ectx.base_ctx.ssl_session = ssl;
        memset(&ectx.base_ctx.client_address, 0, sizeof(ectx.base_ctx.client_address));
        strncpy(ectx.public_dir, pool->public_dir, sizeof(ectx.public_dir) - 1);
        ectx.public_dir[sizeof(ectx.public_dir) - 1] = '\0';

        HttpRequest *req = malloc(sizeof(HttpRequest));
        if (req) {
            char buffer[BUFFER_SIZE];
            bool keep_running = true;

            // 4. Boucle persistante Keep-Alive sur couche TLS (SSL_read)
            while (keep_running) {
                // Remise à zéro complète du tampon à chaque itération Keep-Alive
                memset(buffer, 0, sizeof(buffer));
                int received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
                
                if (received > 0) {
                    buffer[received] = '\0';
                    
                    if (parse_http_request(buffer, req) == 0) {
                        
                        // [vx.x.x] Accumulation stricte du Body pour les requêtes volumineuses (POST)
                        if (req->method == METHOD_POST && req->content_length > 0) {
                            long body_received = 0;
                            
                            if (req->body_start) {
                                // Calcul de la taille du fragment initial du body capturé au premier read
                                body_received = received - (req->body_start - buffer);
                            }
                            
                            // Tant qu'il manque des octets par rapport au Content-Length attendu
                            while (body_received < req->content_length && received < (BUFFER_SIZE - 1)) {
                                int chunk = SSL_read(ssl, buffer + received, sizeof(buffer) - 1 - received);
                                if (chunk <= 0) {
                                    break; // Rupture de flux ou timeout
                                }
                                received += chunk;
                                buffer[received] = '\0';
                                body_received += chunk;
                            }
                        }

                        // Routage avec l'assurance d'avoir collecté l'intégralité de la charge utile
                        handle_http_route(&ectx, req, buffer, received);
                        
                        if (!req->keep_alive) {
                            keep_running = false;
                        }
                    } else {
                        // Option 1 sélectionnée : Transmission chiffrée de l'erreur 400
                        send_ssl_http_error(ssl, 400, "Bad Request", "Malformed HTTP request protocol.", false);
                        keep_running = false;
                    }
                } else {
                    // Fin de flux ou Timeout d'inactivité atteint
                    keep_running = false;
                }
            }
            free(req);
        }

        // 5. Libération et découplage de la session TLS
        SSL_shutdown(ssl);
        SSL_free(ssl);
        lith_close_socket(client_socket);
    }

    return NULL;
}

/**
 * Envoie une réponse d'erreur HTTP structurée à travers le tunnel chiffré SSL
 */
void send_ssl_http_error(SSL *ssl, int status_code, const char *status_text, const char *description, bool keep_alive) {
    char body[512];
    char header[512];
    
    snprintf(body, sizeof(body),
             "<html><head><title>%d %s</title></head>"
             "<body bgcolor=\"#121212\" text=\"#ffffff\"><center><h1>LITH Engine: %d %s</h1><p>%s</p><hr><i>v%s</i></center></body></html>",
             status_code, status_text, status_code, status_text, description, LITH_VERSION);

    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Server: LITH/%s\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "Content-Length: %zu\r\n"
             "Connection: %s\r\n\r\n",
             status_code, status_text, LITH_VERSION, strlen(body), keep_alive ? "keep-alive" : "close");

    SSL_write(ssl, header, (int)strlen(header));
    SSL_write(ssl, body, (int)strlen(body));
}