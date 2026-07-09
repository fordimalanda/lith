/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef LITH_CACHE_H
#define LITH_CACHE_H

/* Macros de contrôle pour exposer les extensions POSIX (pthread_rwlock_t) sous -std=c11 Linux */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_CACHE_ENTRIES 32
#define MAX_CACHE_FILE_SIZE (1024 * 1024) // 1 Mo max par fichier pour protéger la RAM

typedef struct {
    char filepath[256];      // Clé d'identification (ex: "public/index.html")
    char *content;           // Contenu du fichier alloué dynamiquement dans la RAM
    size_t size;             // Taille du fichier en octets
    char mime_type[64];      // MIME-type pré-calculé
    bool is_active;
} CacheEntry;

typedef struct {
    CacheEntry entries[MAX_CACHE_ENTRIES];
    int entry_count;
    pthread_rwlock_t rwlock; // Verrou Lecture/Écriture pour performances max
} LithCache;

/* API du sous-système de Cache */
void lith_cache_init(LithCache *cache);
int lith_cache_add(LithCache *cache, const char *filepath, const char *mime_type);
CacheEntry *lith_cache_lookup(LithCache *cache, const char *filepath);
void lith_cache_destroy(LithCache *cache);

#endif // LITH_CACHE_H