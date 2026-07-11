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
    FILE *file = fopen(filename, "r");
    if (!file) {
        lith_log(LOG_WARN, "Configuration file '%s' not found. Using internal defaults.", filename);
        // Valeurs par défaut sécurisées
        config->port = 8090;
        strncpy(config->public_dir, "public", sizeof(config->public_dir) - 1);
        config->use_cache = 1;
        strncpy(config->ssl_cert_path, "certs/server.crt", sizeof(config->ssl_cert_path) - 1);
        strncpy(config->ssl_key_path, "certs/server.key", sizeof(config->ssl_key_path) - 1);
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Ignorer les commentaires et lignes vides
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        char *delimiter = strchr(line, '=');
        if (!delimiter) continue;

        *delimiter = '\0';
        char *key = line;
        char *value = delimiter + 1;

        // Trim des espaces au début de la clé et de la valeur
        while (isspace((unsigned char)*key)) key++;
        while (isspace((unsigned char)*value)) value++;

        // Trim strict en fin de chaîne (nettoyage crucial de \r, \n et espaces)
        size_t key_len = strlen(key);
        while (key_len > 0 && isspace((unsigned char)key[key_len - 1])) {
            key[key_len - 1] = '\0';
            key_len--;
        }

        size_t val_len = strlen(value);
        while (val_len > 0 && isspace((unsigned char)value[val_len - 1])) {
            value[val_len - 1] = '\0';
            val_len--;
        }

        // Assignation dans la structure globale
        if (strcmp(key, "port") == 0) {
            config->port = atoi(value);
        } else if (strcmp(key, "public_dir") == 0) {
            strncpy(config->public_dir, value, sizeof(config->public_dir) - 1);
            config->public_dir[sizeof(config->public_dir) - 1] = '\0';
        } else if (strcmp(key, "use_cache") == 0) {
            config->use_cache = atoi(value);
        } else if (strcmp(key, "ssl_cert_path") == 0 || strcmp(key, "ssl_cert_file") == 0) {
            strncpy(config->ssl_cert_path, value, sizeof(config->ssl_cert_path) - 1);
            config->ssl_cert_path[sizeof(config->ssl_cert_path) - 1] = '\0';
        } else if (strcmp(key, "ssl_key_path") == 0 || strcmp(key, "ssl_key_file") == 0) {
            strncpy(config->ssl_key_path, value, sizeof(config->ssl_key_path) - 1);
            config->ssl_key_path[sizeof(config->ssl_key_path) - 1] = '\0';
        }
    }

    fclose(file);
    lith_log(LOG_INFO, "Configuration loaded successfully: Port=%d, Public Directory='%s', Cache=%s", 
              config->port, config->public_dir, config->use_cache ? "ENABLED" : "DISABLED");
    return 0;
}