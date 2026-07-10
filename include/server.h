/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int socket_t;
#endif

#include <pthread.h>
#include <stdbool.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "common.h" 
#include "cache.h"  

#define DEFAULT_PORT 8090
#define BACKLOG 10
#define BUFFER_SIZE 4096

/* API de contrôle d'infrastructure - Mode Démon Linux */
#ifndef _WIN32
int lith_daemonize(const char *initial_cwd);
#endif

/* API d'initialisation et d'accès au moteur SSL d'OpenSSL */
int lith_ssl_init(const ServerConfig *config);
SSL_CTX *lith_ssl_get_context(void);
void lith_ssl_cleanup(void);

/* Structures de contexte passées aux routeurs réseau utilisant SSL */
typedef struct {
    socket_t client_socket;
    struct sockaddr_in client_address;
    SSL *ssl_session;
} ClientContext;

typedef struct {
    ClientContext base_ctx;
    char public_dir[256];
} ExpandedClientContext;

/* Structures de synchronisation et de gestion de la file d'attente (Thread Pool) */
typedef struct Node {
    socket_t socket;
    struct Node *next;
} Node_t;

typedef struct {
    Node_t *head;           
    Node_t *tail;           
    int size;               
    pthread_mutex_t mutex;  
    pthread_cond_t cond;    
    bool shutdown;          
    char public_dir[256];   
    LithCache ram_cache;    
} ThreadPool_t;

/* Cycle de vie du serveur réseau */
int lith_init_server(const ServerConfig *config);
void lith_start_server(int server_fd, const ServerConfig *config);

/* Cœur d'exécution asynchrone des Workers */
void *lith_client_handler(void *arg);

/* API de contrôle du Thread Pool */
void thread_pool_init(ThreadPool_t *pool, int num_threads, const char *public_dir);
void thread_pool_push(ThreadPool_t *pool, socket_t socket);
socket_t thread_pool_pop(ThreadPool_t *pool);
void thread_pool_shutdown(ThreadPool_t *pool);

#endif // SERVER_H