/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "http_router.h"
#include "server_utils.h"
#include "logger.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern ThreadPool_t global_pool; // Récupération du pool global contenant le cache

/**
 * Envoie une page d'erreur HTML stylisée en respectant l'état Keep-Alive demandé
 */
void send_http_error(socket_t client_socket, int status_code, const char *status_text, const char *description, bool keep_alive) {
    const char *html = get_error_html(status_code, status_text, description);
    char header[384];
    
    // Si c'est un 400 Bad Request, le protocole est corrompu -> fermeture forcée.
    // Sinon, on respecte scrupuleusement la volonté du cycle Keep-Alive négocié.
    const char *conn_state = (status_code == 400) ? "close" : (keep_alive ? "keep-alive" : "close");

    snprintf(header, sizeof(header), 
             "HTTP/1.1 %d %s\r\n"
             "Content-Length: %zu\r\n"
             "Content-Type: text/html\r\n"
             "Connection: %s\r\n\r\n", 
             status_code, status_text, strlen(html), conn_state);
             
    send(client_socket, header, (int)strlen(header), 0);
    send(client_socket, html, (int)strlen(html), 0);
}

/**
 * Routeur HTTP principal de LITH - Gère les réponses dynamiques API et le cache RAM statique
 */
void handle_http_route(ExpandedClientContext *ectx, HttpRequest *req, char *full_buffer, int total_received) {
    ClientContext *ctx = &(ectx->base_ctx);
    (void)full_buffer; 
    (void)total_received;

    // Détermination dynamique du statut de connexion à renvoyer au client
    const char *conn_state = req->keep_alive ? "keep-alive" : "close";

    // --- ROUTE API : POST ---
    if (req->method == METHOD_POST) {
        lith_log(LOG_INFO, "POST Payload size: %ld bytes", req->content_length);
        
        char response_body[512];
        snprintf(response_body, sizeof(response_body),
                 "{\"status\":\"success\",\"message\":\"Data received successfully by LITH\",\"bytes_processed\":%ld}",
                 req->content_length);

        char header[384];
        snprintf(header, sizeof(header), 
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %zu\r\n"
                 "Content-Type: application/json\r\n"
                 "Connection: %s\r\n"
                 "Keep-Alive: timeout=3, max=100\r\n\r\n", 
                 strlen(response_body), conn_state);

        send(ctx->client_socket, header, (int)strlen(header), 0);
        send(ctx->client_socket, response_body, (int)strlen(response_body), 0);
    } 
    // --- ROUTE FICHIERS STATIQUES : GET ---
    else if (req->method == METHOD_GET) {
        if (!is_safe_path(req->path)) {
            lith_log(LOG_WARN, "Security Alert: Blocked traversal attempt on path: %s", req->path);
            send_http_error(ctx->client_socket, 403, "Forbidden", "Access to this resource is strictly prohibited.", req->keep_alive);
            return;
        }

        // 1. Résolution du chemin local du fichier demandé
        char file_path[512];
        strncpy(file_path, ectx->public_dir, sizeof(file_path) - 1);
        file_path[sizeof(file_path) - 1] = '\0';

        if (strcmp(req->path, "/") == 0) {
            strcat(file_path, "/index.html");
        } else {
            strcat(file_path, req->path);
        }

        // 2. INTERCEPTION : Recherche directe dans le sous-système de Cache RAM
        CacheEntry *cached = lith_cache_lookup(&global_pool.ram_cache, file_path);
        
        if (cached != NULL) {
            // AJOUT LOG : Suivi des Hits du Cache RAM
            lith_log(LOG_INFO, "200 OK (RAM Cache hit) - Served: %s", req->path);

            char header[384];
            snprintf(header, sizeof(header), 
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Length: %zu\r\n"
                     "Content-Type: %s\r\n"
                     "Connection: %s\r\n"
                     "Keep-Alive: timeout=3, max=100\r\n\r\n", 
                     cached->size, cached->mime_type, conn_state);
                     
            send(ctx->client_socket, header, (int)strlen(header), 0);
            send(ctx->client_socket, cached->content, (int)cached->size, 0);
            
            pthread_rwlock_unlock(&global_pool.ram_cache.rwlock);
            return;
        }

        // --- MISS CACHE : Fallback disque traditionnel ---
        long file_size = 0;
        char *file_content = read_file(file_path, &file_size);

        if (file_content) {
            // AJOUT LOG : Suivi des Miss / Lectures depuis le Disque
            lith_log(LOG_INFO, "200 OK (Disk Fallback) - Served: %s", req->path);

            char header[384];
            const char *mime_type = get_mime_type(file_path);
            
            snprintf(header, sizeof(header), 
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Length: %ld\r\n"
                     "Content-Type: %s\r\n"
                     "Connection: %s\r\n"
                     "Keep-Alive: timeout=3, max=100\r\n\r\n", 
                     file_size, mime_type, conn_state);
                     
            send(ctx->client_socket, header, (int)strlen(header), 0);
            send(ctx->client_socket, file_content, (int)file_size, 0);
            free(file_content);
        } else {
            lith_log(LOG_WARN, "404 - Not Found: %s", file_path);
            send_http_error(ctx->client_socket, 404, "Not Found", "The requested URL or resource could not be located on this server.", req->keep_alive);
        }
    }
}