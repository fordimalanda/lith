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
 * Retourne 0 en cas de succès, -1 en cas d'erreur critique.
 */
int lith_daemonize(void) {
    pid_t pid;

    // 1. Premier fork : Séparation du processus parent
    pid = fork();
    if (pid < 0) {
        lith_log(LOG_ERROR, "Daemonization CRITICAL: First fork failed");
        return -1;
    }
    // Si nous sommes le parent, on quitte immédiatement pour rendre la main au shell
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
        exit(0); // Le premier enfant meurt, le petit-enfant devient le démon final
    }

    // 4. Réinitialisation du masque de fichier (umask)
    umask(0);

    // 5. Changement vers le répertoire racine pour ne pas bloquer les volumes de l'OS
    if (chdir("/") < 0) {
        lith_log(LOG_ERROR, "Daemonization CRITICAL: chdir to root partition failed");
        return -1;
    }

    // 6. Fermeture des descripteurs standards du terminal
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirection de STDIN vers l'oubliette du système
    int dev_null = open("/dev/null", O_RDONLY);
    if (dev_null < 0) return -1;

    // Redirection de STDOUT et STDERR vers lith.log
    // O_APPEND garantit que les redémarrages successifs écrivent à la suite des logs
    int log_fd = open("lith.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        // Fallback de sécurité si le répertoire actuel n'est pas scriptable
        log_fd = open("/tmp/lith.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    }

    if (log_fd >= 0) {
        // Redirection de stdout (1) et stderr (2) vers le fichier de log via duplication
        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
    }

    lith_log(LOG_INFO, "LITH Engine daemonized successfully. Operating silently in background.");
    return 0;
}
#endif