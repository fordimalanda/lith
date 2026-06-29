/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "common.h"
#include "server.h"
#include "logger.h"
#include <signal.h>

void handle_sigint(int sig) {
    (void)sig;
    printf("\n");
    lith_log(LOG_INFO, "Shutting down LITH server...");
    
#ifdef _WIN32
    // Nettoyage de Winsock effectué proprement juste avant la destruction du processus
    WSACleanup();
#endif

    exit(0);
}

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;
    signal(SIGINT, handle_sigint);

    printf("----------------------------------------\n");
    printf("   LITH : Light HTTP Server v%s\n", LITH_VERSION);
    printf("   Status: Running\n");
    printf("----------------------------------------\n");

    int server_fd = lith_init_server(port);
    
    // Traitement strict de l'échec du bind/initialisation
    if (server_fd < 0) {
        lith_log(LOG_ERROR, "Failed to start server. Port %d might already be in use.", port);
        return 1; // Quitte proprement avec un code d'erreur non-nul (exit 1)
    }

    lith_start_server(server_fd);
    return 0;
}