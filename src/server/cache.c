/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "cache.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // <--- Requis pour stat

void lith_cache_init(LithCache *cache) {
    cache->entry_count = 0;
    pthread_rwlock_init(&cache->rwlock, NULL);
    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        cache->entries[i].is_active = false;
        cache->entries[i].content = NULL;
        cache->entries[i].last_modified = 0;
    }
    lith_log(LOG_INFO, "RAM Cache subsystem initialized seamlessly.");
}

int lith_cache_add(LithCache *cache, const char *filepath, const char *mime_type) {
    pthread_rwlock_wrlock(&cache->rwlock); // Verrouillage en ÉCRITURE

    if (cache->entry_count >= MAX_CACHE_ENTRIES) {
        pthread_rwlock_unlock(&cache->rwlock);
        return -1;
    }

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        pthread_rwlock_unlock(&cache->rwlock);
        return -2; // Fichier introuvable
    }

    // Récupération des stats du fichier (taille et date)
    struct stat st;
    if (stat(filepath, &st) != 0) {
        fclose(file);
        pthread_rwlock_unlock(&cache->rwlock);
        return -5;
    }

    if (st.st_size > MAX_CACHE_FILE_SIZE) {
        fclose(file);
        pthread_rwlock_unlock(&cache->rwlock);
        lith_log(LOG_WARN, "Cache skip: File %s too large for RAM boundaries", filepath);
        return -3;
    }

    char *buffer = malloc(st.st_size);
    if (!buffer) {
        fclose(file);
        pthread_rwlock_unlock(&cache->rwlock);
        return -4;
    }

    size_t read_bytes = fread(buffer, 1, st.st_size, file);
    fclose(file);

    int idx = cache->entry_count;
    strncpy(cache->entries[idx].filepath, filepath, 255);
    cache->entries[idx].filepath[255] = '\0';
    cache->entries[idx].content = buffer;
    cache->entries[idx].size = read_bytes;
    strncpy(cache->entries[idx].mime_type, mime_type, 63);
    cache->entries[idx].mime_type[63] = '\0';
    cache->entries[idx].last_modified = st.st_mtime; // <--- Sauvegarde du timestamp
    cache->entries[idx].is_active = true;

    cache->entry_count++;
    pthread_rwlock_unlock(&cache->rwlock); // Libération du verrou

    lith_log(LOG_INFO, "Cached static resource: %s (%zu bytes mapped to RAM)", filepath, read_bytes);
    return 0;
}

CacheEntry *lith_cache_lookup(LithCache *cache, const char *filepath) {
    pthread_rwlock_rdlock(&cache->rwlock); // Verrouillage en LECTURE

    for (int i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i].is_active && strcmp(cache->entries[i].filepath, filepath) == 0) {
            // Le thread appelant garde le verrou de lecture pour protéger les données
            return &cache->entries[i];
        }
    }

    pthread_rwlock_unlock(&cache->rwlock);
    return NULL;
}

/**
 * [OPTION 2] Rechargement à chaud d'une entrée de cache obsolète (Thread-safe)
 * Doit être appelé sous verrou d'ÉCRITURE exclusif !
 */
bool lith_cache_hot_reload(CacheEntry *entry) {
    FILE *file = fopen(entry->filepath, "rb");
    if (!file) return false;

    struct stat st;
    if (stat(entry->filepath, &st) != 0) {
        fclose(file);
        return false;
    }

    char *new_content = malloc(st.st_size);
    if (!new_content) {
        fclose(file);
        return false;
    }

    size_t read_bytes = fread(new_content, 1, st.st_size, file);
    fclose(file);

    // Swap des mémoires
    free(entry->content);
    entry->content = new_content;
    entry->size = read_bytes;
    entry->last_modified = st.st_mtime;

    lith_log(LOG_INFO, "[HOT-RELOAD] RAM Cache updated dynamically for: %s (%zu bytes updated)", entry->filepath, read_bytes);
    return true;
}

void lith_cache_destroy(LithCache *cache) {
    pthread_rwlock_wrlock(&cache->rwlock);
    for (int i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i].content) {
            free(cache->entries[i].content);
        }
    }
    pthread_rwlock_destroy(&cache->rwlock);
}