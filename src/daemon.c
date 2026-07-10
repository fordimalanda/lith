/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef _WIN32
#include "server.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * Détache le processus de la session du terminal pour en faire un démon système autonome.
 * Utilise le chemin d'exécution initial pour y ancrer son fichier de log.
 */
int lith_daemonize(const char *initial_cwd) {
    pid_t pid;

    // 1. Premier fork : Séparation du processus parent
    pid = fork();
    if (pid < 0) {
        lith_log(LOG_ERROR, "Daemonization CRITICAL: First fork failed");
        return -1;
    }
    if (pid > 0) {
        exit(0); 
    }

    // 2. Création d'une nouvelle session et isolation complète du terminal
    if (setsid() < 0) {
        lith_log(LOG_ERROR, "Daemonization CRITICAL: setsid failed to detach session");
        return -1;
    }

    // 3. Deuxième fork : Empêche le processus de réacquérir un terminal de contrôle
    pid = fork();
    if (pid < 0) {
        lith_log(LOG_ERROR, "Daemonization CRITICAL: Second fork failed");
        return -1;
    }
    if (pid > 0) {
        exit(0); 
    }

    // 4. Réinitialisation du masque de fichier (umask)
    umask(0);

    // Construction du chemin absolu pour le fichier de log avant le chdir
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/lith.log", initial_cwd);

    // 5. Changement vers le répertoire racine pour ne pas bloquer les volumes de l'OS
    if (chdir("/") < 0) {
        lith_log(LOG_ERROR, "Daemonization CRITICAL: chdir to root partition failed");
        return -1;
    }

    // 6. Fermeture des descripteurs standards du terminal
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirection de STDIN vers /dev/null
    int dev_null = open("/dev/null", O_RDONLY);
    if (dev_null < 0) return -1;

    // Redirection de STDOUT et STDERR vers le chemin absolu calculé de lith.log
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        // Fallback de sécurité si l'emplacement initial n'est pas accessible en écriture
        log_fd = open("/tmp/lith.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    }

    if (log_fd >= 0) {
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
    }

    lith_log(LOG_INFO, "LITH Engine daemonized successfully. Operating silently in background.");
    return 0;
}
#endif