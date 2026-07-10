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
#include <sys/stat.h> 
#include <openssl/ssl.h>

extern ThreadPool_t global_pool;
extern ServerConfig global_config; 

void send_http_error(SSL *ssl, int status_code, const char *status_text, const char *description, bool keep_alive) {
    const char *html = get_error_html(status_code, status_text, description);
    char header[384];
    const char *conn_state = (status_code == 400) ? "close" : (keep_alive ? "keep-alive" : "close");

    snprintf(header, sizeof(header), 
             "HTTP/1.1 %d %s\r\n"
             "Content-Length: %zu\r\n"
             "Content-Type: text/html\r\n"
             "Connection: %s\r\n\r\n", 
             status_code, status_text, strlen(html), conn_state);
             
    SSL_write(ssl, header, (int)strlen(header));
    SSL_write(ssl, html, (int)strlen(html));
}

void handle_http_route(ExpandedClientContext *ectx, HttpRequest *req, char *full_buffer, int total_received) {
    SSL *ssl = ectx->base_ctx.ssl_session;
    (void)full_buffer; 
    (void)total_received;

    const char *conn_state = req->keep_alive ? "keep-alive" : "close";

    if (req->method == METHOD_POST) {
        lith_log(LOG_INFO, "POST Payload size: %ld bytes", req->content_length);
        char response_body[512];
        snprintf(response_body, sizeof(response_body),
                 "{\"status\":\"success\",\"message\":\"Data received successfully secure by LITH\",\"bytes_processed\":%ld}",
                 req->content_length);

        char header[384];
        snprintf(header, sizeof(header), 
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %zu\r\n"
                 "Content-Type: application/json\r\n"
                 "Connection: %s\r\n"
                 "Keep-Alive: timeout=3, max=100\r\n\r\n", 
                 strlen(response_body), conn_state);

        SSL_write(ssl, header, (int)strlen(header));
        SSL_write(ssl, response_body, (int)strlen(response_body));
    } 
    else if (req->method == METHOD_GET) {
        if (!is_safe_path(req->path)) {
            lith_log(LOG_WARN, "Security Alert: Blocked traversal attempt on path: %s", req->path);
            send_http_error(ssl, 403, "Forbidden", "Access to this resource is strictly prohibited.", req->keep_alive);
            return;
        }

        char file_path[512];
        strncpy(file_path, ectx->public_dir, sizeof(file_path) - 1);
        file_path[sizeof(file_path) - 1] = '\0';

        if (strcmp(req->path, "/") == 0) {
            strcat(file_path, "/index.html");
        } else {
            strcat(file_path, req->path);
        }

        if (global_config.use_cache == 1) {
            CacheEntry *cached = lith_cache_lookup(&global_pool.ram_cache, file_path);
            
            if (cached != NULL) {
                struct stat st;
                if (stat(file_path, &st) == 0) {
                    if (st.st_mtime > cached->last_modified) {
                        pthread_rwlock_unlock(&global_pool.ram_cache.rwlock);
                        
                        pthread_rwlock_wrlock(&global_pool.ram_cache.rwlock);
                        lith_cache_hot_reload(cached);
                        pthread_rwlock_unlock(&global_pool.ram_cache.rwlock);
                        
                        pthread_rwlock_rdlock(&global_pool.ram_cache.rwlock);
                    }
                }

                lith_log(LOG_INFO, "200 OK (RAM Cache hit) - Served: %s", req->path);

                char header[384];
                snprintf(header, sizeof(header), 
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Length: %zu\r\n"
                         "Content-Type: %s\r\n"
                         "Connection: %s\r\n"
                         "Keep-Alive: timeout=3, max=100\r\n\r\n", 
                         cached->size, cached->mime_type, conn_state);
                         
                SSL_write(ssl, header, (int)strlen(header));
                SSL_write(ssl, cached->content, (int)cached->size);
                
                pthread_rwlock_unlock(&global_pool.ram_cache.rwlock);
                return;
            }
        }

        long file_size = 0;
        char *file_content = read_file(file_path, &file_size);

        if (file_content) {
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
                     
            SSL_write(ssl, header, (int)strlen(header));
            SSL_write(ssl, file_content, (int)file_size);
            free(file_content);
        } else {
            lith_log(LOG_WARN, "404 - Not Found: %s", file_path);
            send_http_error(ssl, 404, "Not Found", "The requested URL or resource could not be located on this server.", req->keep_alive);
        }
    }
}