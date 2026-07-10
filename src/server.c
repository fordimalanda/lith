/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "server.h"
#include "config.h"
#include "common.h"
#include "server_utils.h"
#include "logger.h"
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

extern ThreadPool_t global_pool;

int lith_init_server(const ServerConfig *config) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        lith_log(LOG_ERROR, "WSAStartup failed");
        return -1;
    }
#endif

    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == (socket_t)-1) {
        lith_log(LOG_ERROR, "Socket creation failed");
        return -1;
    }

    int opt = 1;
#ifdef _WIN32
    if (setsockopt(server_fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&opt, sizeof(opt)) < 0) {
        lith_log(LOG_WARN, "Failed to set SO_EXCLUSIVEADDRUSE");
    }
#else
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        lith_log(LOG_WARN, "Failed to set SO_REUSEADDR");
    }
#endif

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config->port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        lith_log(LOG_ERROR, "Bind failed on port %d. Port might already be in use.", config->port);
        lith_close_socket(server_fd);
        return -1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        lith_log(LOG_ERROR, "Listen failed");
        lith_close_socket(server_fd);
        return -1;
    }

    /* Initialisation obligatoire du sous-système SSL */
    if (lith_ssl_init(config) < 0) {
        lith_log(LOG_ERROR, "Critical structural error: Cryptographic layer block failure");
        lith_close_socket(server_fd);
        return -1;
    }

    lith_log(LOG_INFO, "LITH Secure HTTP Engine (%s) ready on port %d", LITH_VERSION, config->port);
    return (int)server_fd;
}

void lith_start_server(int server_fd, const ServerConfig *config) {
    (void)config; 
    lith_log(LOG_INFO, "LITH core engine event loop started. Waiting for safe incoming connections...");

    while (true) {
        struct sockaddr_in addr;
#ifdef _WIN32
        int len = sizeof(addr);
#else
        socklen_t len = sizeof(addr);
#endif
        
        socket_t client_socket = accept((socket_t)server_fd, (struct sockaddr *)&addr, &len);

        if (client_socket == (socket_t)-1) {
            if (global_pool.shutdown) {
                break;
            }
            continue;
        }

        thread_pool_push(&global_pool, client_socket);
    }
}