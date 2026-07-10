/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "server.h"
#include "server_utils.h"
#include "logger.h"
#include "config.h"
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <limits.h>

#ifndef _WIN32
#include <unistd.h>
#endif

ThreadPool_t global_pool;
ServerConfig global_config; 
int global_server_fd = -1;  

void handle_sigint(int sig) {
    (void)sig;
    printf("\n");
    lith_log(LOG_INFO, "Initiating LITH orderly shutdown sequence...");
    
    if (global_server_fd != -1) {
        lith_close_socket(global_server_fd);
    }
    
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
        // 1. Sauvegarde du répertoire de travail initial avant isolation
        char initial_cwd[512] = {0};
#ifndef _WIN32
        if (getcwd(initial_cwd, sizeof(initial_cwd)) == NULL) {
            lith_log(LOG_ERROR, "Failed to get current working directory");
            return 1;
        }
#endif

        // 2. Lecture impérative de la configuration
        load_config("lith.conf", &global_config);

        int port_override = 0;
        bool use_daemon = false;

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
            global_config.port = port_override;
        }

        // --- ENCLENCHEMENT DU MODE DÉMON LINUX (Résolution des chemins absolus) ---
        if (use_daemon) {
#ifdef _WIN32
            lith_log(LOG_WARN, "Daemon mode (-d/--daemon) is not supported natively on Windows. Falling back to foreground.");
#else
            // Résolution dynamique du dossier public vers un chemin absolu
            if (global_config.public_dir[0] != '/') {
                char absolute_public[512];
                snprintf(absolute_public, sizeof(absolute_public), "%s/%s", initial_cwd, global_config.public_dir);
                strncpy(global_config.public_dir, absolute_public, sizeof(global_config.public_dir) - 1);
                global_config.public_dir[sizeof(global_config.public_dir) - 1] = '\0';
            }

            lith_log(LOG_INFO, "Detaching session. LITH is shifting control to background daemon...");
            
            // On transmet le chemin d'origine pour l'ancrage correct du fichier lith.log
            if (lith_daemonize(initial_cwd) < 0) {
                lith_log(LOG_ERROR, "Critical: Daemonization execution routine failed");
                return 1;
            }
#endif
        }

        printf("----------------------------------------\n");
        printf("   LITH : Light HTTP Server v%s\n", LITH_VERSION);
        printf("   Status: Initializing Architecture...\n");
        printf("----------------------------------------\n");

        thread_pool_init(&global_pool, 8, global_config.public_dir);

        int server_fd = lith_init_server(&global_config);
        if (server_fd < 0) {
            lith_log(LOG_ERROR, "Critical: Failed to initialize network socket layer");
            thread_pool_shutdown(&global_pool);
            return 1;
        }

        global_server_fd = server_fd; 

        lith_start_server(server_fd, &global_config);
        return 0;
    }

    fprintf(stderr, "Error: Unknown command or option '%s'.\n\n", argv[1]);
    display_usage();
    return 1;
}