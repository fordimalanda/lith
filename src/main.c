/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "server.h"
#include "logger.h"
#include "config.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

ThreadPool_t global_pool;
int global_server_fd = -1; // Référence globale pour libérer accept() lors du Ctrl+C

void handle_sigint(int sig) {
    (void)sig;
    printf("\n");
    lith_log(LOG_INFO, "Initiating LITH orderly shutdown sequence...");
    
    // 1. Fermeture forcée du socket d'écoute pour débloquer immédiatement accept()
    if (global_server_fd != -1) {
        lith_close_socket(global_server_fd);
    }
    
    // 2. Arrêt propre des workers
    thread_pool_shutdown(&global_pool);

#ifdef _WIN32
    WSACleanup();
#endif

    lith_log(LOG_INFO, "LITH server engine stopped successfully. Goodbye.");
    exit(0);
}

void display_usage() {
    printf("======================================================================\n");
    printf(" LITH CLI - Minimalist High-Performance HTTP Engine v%s\n", LITH_VERSION);
    printf("======================================================================\n");
    printf("Usage: lith [command] [options]\n\n");
    printf("Commands:\n");
    printf("  start               Explicitly launch the HTTP server engine\n\n");
    printf("Options:\n");
    printf("  -d, --daemon        Run the server in the background (Linux only)\n");
    printf("  -v, --version       Print current stable release version\n");
    printf("  -h, --help          Display available flags and routing commands\n\n");
    printf("Examples:\n");
    printf("  lith start 8080       Launch in foreground on port 8080\n");
    printf("  lith start 8080 -d    Launch as a background daemon process\n");
    printf("----------------------------------------------------------------------\n");
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);

    if (argc < 2) {
        display_usage();
        return 0;
    }

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("LITH Engine Version: %s\n", LITH_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        display_usage();
        return 0;
    }

    if (strcmp(argv[1], "start") == 0) {
        ServerConfig config;
        // 1. Lecture impérative de la config avant de faire chdir("/") dans la démonisation
        load_config("lith.conf", &config);

        int port_override = 0;
        bool use_daemon = false;

        // Analyse des flags additionnels : lith start [port] [--daemon / -d]
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--daemon") == 0 || strcmp(argv[i], "-d") == 0) {
                use_daemon = true;
            } else {
                int val = atoi(argv[i]);
                if (val > 0) {
                    port_override = val;
                }
            }
        }

        if (port_override > 0) {
            lith_log(LOG_INFO, "CLI Argument detected: Overriding port to %d", port_override);
            config.port = port_override;
        }

        // --- ENCLENCHEMENT DU MODE DÉMON LINUX v1.0.8 ---
        if (use_daemon) {
#ifdef _WIN32
            lith_log(LOG_WARN, "Daemon mode (-d/--daemon) is not supported natively on Windows. Falling back to foreground.");
#else
            lith_log(LOG_INFO, "Detaching session. LITH is shifting control to background daemon...");
            if (lith_daemonize() < 0) {
                lith_log(LOG_ERROR, "Critical: Daemonization execution routine failed");
                return 1;
            }
#endif
        }

        printf("----------------------------------------\n");
        printf("   LITH : Light HTTP Server v%s\n", LITH_VERSION);
        printf("   Status: Initializing Architecture...\n");
        printf("----------------------------------------\n");

        thread_pool_init(&global_pool, 8, config.public_dir);

        int server_fd = lith_init_server(&config);
        if (server_fd < 0) {
            lith_log(LOG_ERROR, "Critical: Failed to initialize network socket layer");
            thread_pool_shutdown(&global_pool);
            return 1;
        }

        global_server_fd = server_fd; // Armement de la référence globale pour le signal SIGINT

        lith_start_server(server_fd, &config);
        return 0;
    }

    fprintf(stderr, "Error: Unknown command or option '%s'.\n\n", argv[1]);
    display_usage();
    return 1;
}