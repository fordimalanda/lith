#ifndef COMMON_H
#define COMMON_H

// Inclusions standards universelles
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

// --- Configuration du Serveur ---
#define LITH_VERSION "0.1.0"
#define LITH_SERVER_NAME "LITH-Embedded-Server"

// --- Sécurité et Limites ---
// Empêche les attaques par déni de service simples ou les dépassements
#define MAX_CONNECTIONS 100
#define READ_TIMEOUT_SEC 5

// --- Macros Utilitaires ---
// Une macro pratique pour vérifier les erreurs système rapidement
#define CHECK_ERROR(condition, msg) \
    do { \
        if (condition) { \
            perror(msg); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#endif