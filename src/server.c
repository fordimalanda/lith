#include "server.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    lith_log(LOG_INFO, "LITH %s ready on port %d", LITH_VERSION, config->port);
    return (int)server_fd;
}

void lith_start_server(int server_fd, const ServerConfig *config) {
    while (true) {
        struct sockaddr_in addr;
#ifdef _WIN32
        int len = sizeof(addr);
#else
        socklen_t len = sizeof(addr);
#endif
        
        ExpandedClientContext *ectx = malloc(sizeof(ExpandedClientContext));
        if (!ectx) continue;

        strncpy(ectx->public_dir, config->public_dir, sizeof(ectx->public_dir) - 1);
        ectx->public_dir[sizeof(ectx->public_dir) - 1] = '\0';

        ectx->base_ctx.client_socket = accept((socket_t)server_fd, (struct sockaddr *)&addr, &len);

        if (ectx->base_ctx.client_socket == (socket_t)-1) {
            free(ectx);
            continue;
        }

        // --- CONFIGURATION DU TIMEOUT SUR LE SOCKET CLIENT (Anti-Slowloris) ---
#ifdef _WIN32
        DWORD timeout = 3000;
        setsockopt(ectx->base_ctx.client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        setsockopt(ectx->base_ctx.client_socket, SOL_SOCKET, SO_RCVTIMEO, (const struct timeval *)&tv, sizeof(tv));
#endif

        pthread_t tid;
        if (pthread_create(&tid, NULL, lith_client_handler, ectx) == 0) {
            pthread_detach(tid);
        } else {
            lith_close_socket(ectx->base_ctx.client_socket);
            free(ectx);
        }
    }
}