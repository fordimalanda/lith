/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int load_config(const char *filename, ServerConfig *config) {
    config->port = 8090;
    strcpy(config->public_dir, "public");
    config->use_cache = 0; 
    memset(config->ssl_cert_path, 0, sizeof(config->ssl_cert_path));
    memset(config->ssl_key_path, 0, sizeof(config->ssl_key_path));

    FILE *f = fopen(filename, "r");
    if (!f) {
        lith_log(LOG_WARN, "Configuration file '%s' not found. Using low-level standalone engine defaults.", filename);
        return -1;
    }

    char line[384];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;

        char *ptr = line;
        while (isspace((unsigned char)*ptr)) ptr++;
        if (*ptr == '\0' || *ptr == '#') continue;

        char *equal = strchr(ptr, '=');
        if (!equal) continue;

        *equal = '\0';
        char *key = ptr;
        char *value = equal + 1;

        while (isspace((unsigned char)*key)) key++;
        char *end_key = key + strlen(key) - 1;
        while (end_key > key && isspace((unsigned char)*end_key)) { *end_key = '\0'; end_key--; }

        while (isspace((unsigned char)*value)) value++;
        char *end_val = value + strlen(value) - 1;
        while (end_val > value && isspace((unsigned char)*end_val)) { *end_val = '\0'; end_val--; }

        if (strcmp(key, "PORT") == 0) {
            config->port = atoi(value);
        } else if (strcmp(key, "PUBLIC_DIR") == 0) {
            strncpy(config->public_dir, value, sizeof(config->public_dir) - 1);
            config->public_dir[sizeof(config->public_dir) - 1] = '\0';
        } else if (strcmp(key, "USE_CACHE") == 0) { 
            config->use_cache = atoi(value);
        } else if (strcmp(key, "SSL_CERT_PATH") == 0) { 
            strncpy(config->ssl_cert_path, value, sizeof(config->ssl_cert_path) - 1);
            config->ssl_cert_path[sizeof(config->ssl_cert_path) - 1] = '\0';
        } else if (strcmp(key, "SSL_KEY_PATH") == 0) { 
            strncpy(config->ssl_key_path, value, sizeof(config->ssl_key_path) - 1);
            config->ssl_key_path[sizeof(config->ssl_key_path) - 1] = '\0';
        }
    }

    fclose(f);
    lith_log(LOG_INFO, "Configuration loaded: Port=%d, Public Directory='%s', Cache=%s", 
             config->port, config->public_dir, config->use_cache ? "ENABLED (Production Mode)" : "DISABLED (Development Mode)");
    return 0;
}