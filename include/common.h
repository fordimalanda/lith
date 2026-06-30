/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#define LITH_VERSION "1.0.5"
#define LITH_SERVER_NAME "LITH-v1"

#define MAX_CONNECTIONS 100
#define READ_TIMEOUT_SEC 5

/* Structure pour stocker la configuration globale externe */
typedef struct {
    int port;
    char public_dir[256];
} ServerConfig;

#define CHECK_ERROR(condition, msg) \
    do { \
        if (condition) { \
            perror(msg); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#endif