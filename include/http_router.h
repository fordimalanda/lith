/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#ifndef HTTP_ROUTER_H
#define HTTP_ROUTER_H

#include "server.h"
#include "http_parser.h"
#include <openssl/ssl.h>

/**
 * Route et traite la requête HTTP reçue à travers la session TLS
 */
void handle_http_route(ExpandedClientContext *ectx, HttpRequest *req, char *full_buffer, int total_received);

/**
 * Envoie une réponse d'erreur HTTP structurée et chiffrée via OpenSSL
 */
void send_http_error(SSL *ssl, int status_code, const char *status_text, const char *description, bool keep_alive);

#endif // HTTP_ROUTER_H