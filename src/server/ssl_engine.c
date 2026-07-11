/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include <openssl/ssl.h>
#include <openssl/err.h>
#include "logger.h"
#include "common.h"
#include <string.h>

static SSL_CTX *global_ssl_ctx = NULL;

int lith_ssl_init(const ServerConfig *config) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD *method = TLS_server_method();
    global_ssl_ctx = SSL_CTX_new(method);
    
    if (!global_ssl_ctx) {
        lith_log(LOG_ERROR, "SSL: Failed to create global context framework");
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Détermination sécurisée des chemins (fallback si les chaînes de config sont vides)
    const char *cert_path = (config->ssl_cert_path && strlen(config->ssl_cert_path) > 0) 
                            ? config->ssl_cert_path 
                            : "certs/server.crt";

    const char *key_path = (config->ssl_key_path && strlen(config->ssl_key_path) > 0) 
                           ? config->ssl_key_path 
                           : "certs/server.key";

    // Chargement du certificat public (.crt / .pem)
    if (SSL_CTX_use_certificate_file(global_ssl_ctx, cert_path, SSL_FILETYPE_PEM) <= 0) {
        lith_log(LOG_ERROR, "SSL: Failed to load cryptographic certificate target: '%s'", cert_path);
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Chargement de la clé privée (.key)
    if (SSL_CTX_use_PrivateKey_file(global_ssl_ctx, key_path, SSL_FILETYPE_PEM) <= 0) {
        lith_log(LOG_ERROR, "SSL: Failed to load cryptographic private key target: '%s'", key_path);
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Vérification de sécurité de l'alignement paire publique / privée
    if (!SSL_CTX_check_private_key(global_ssl_ctx)) {
        lith_log(LOG_ERROR, "SSL: Structural validation failed. Private key mismatch with public certificate.");
        return -1;
    }

    lith_log(LOG_INFO, "SSL: Cryptographic engine initialized securely with modern TLS.");
    return 0;
}

SSL_CTX *lith_ssl_get_context(void) {
    return global_ssl_ctx;
}

void lith_ssl_cleanup(void) {
    if (global_ssl_ctx) {
        SSL_CTX_free(global_ssl_ctx);
        global_ssl_ctx = NULL;
    }
    EVP_cleanup();
}