/* * Copyright (c) 2026 Fordi / FomaDev. 
 * Licensed under FomaDev Public License.
 * See LICENSE file in the project root for full license information.
 */

#include "http_parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Converts HttpMethod enum values to their corresponding string representations
 */
const char* method_to_str(HttpMethod method) {
    switch (method) {
        case METHOD_GET:  return "GET";
        case METHOD_POST: return "POST";
        default:          return "UNKNOWN";
    }
}

/**
 * Parses a raw HTTP request string into an HttpRequest structure
 * Returns 0 on success, -1 on failure
 */
int parse_http_request(const char *raw_request, HttpRequest *out_request) {
    if (!raw_request || !out_request) return -1;

    // Réinitialisation de la structure cible
    memset(out_request, 0, sizeof(HttpRequest));
    out_request->method = METHOD_UNKNOWN;

    char method_str[16] = {0};
    char path_str[MAX_PATH_SIZE] = {0};

    // Lecture de la Request-Line (Méthode et Chemin)
    if (sscanf(raw_request, "%15s %255s", method_str, path_str) < 2) {
        return -1;
    }

    // Identification de la méthode HTTP
    if (strcmp(method_str, "GET") == 0) {
        out_request->method = METHOD_GET;
    } else if (strcmp(method_str, "POST") == 0) {
        out_request->method = METHOD_POST;
    } else {
        out_request->method = METHOD_UNKNOWN;
    }

    // Copie sécurisée du chemin d'accès
    strncpy(out_request->path, path_str, MAX_PATH_SIZE - 1);
    out_request->path[MAX_PATH_SIZE - 1] = '\0';

    // Recherche basique du corps (Body) si présent (utile pour POST)
    const char *body_start = strstr(raw_request, "\r\n\r\n");
    if (body_start) {
        body_start += 4; // Avance après les caractères de fin d'en-tête
        strncpy(out_request->body, body_start, MAX_BODY_SIZE - 1);
        out_request->body[MAX_BODY_SIZE - 1] = '\0';
        out_request->content_length = (int)strlen(out_request->body);
    }

    return 0;
}

/**
 * Extracts the file extension and returns the corresponding MIME type
 * Defaults to "application/octet-stream" if unknown or missing
 */
const char* get_mime_type(const char *path) {
    if (!path) return "application/octet-stream";

    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0)  return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".txt") == 0) return "text/plain";

    return "application/octet-stream";
}

/**
 * Generates a centralized styled HTML error page
 * Returns a pointer to a static buffer containing the HTML
 */
const char* get_error_html(int status_code, const char *title, const char *message) {
    static char error_buffer[1024];
    
    snprintf(error_buffer, sizeof(error_buffer),
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>%d - %s</title>\n"
        "    <style>\n"
        "        body { font-family: 'Segoe UI', system-ui, sans-serif; background: #0f172a; color: #f8fafc; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; }\n"
        "        .error-card { text-align: center; background: #1e293b; padding: 3rem; border-radius: 12px; box-shadow: 0 10px 25px rgba(0,0,0,0.3); max-width: 450px; width: 90%%; border: 1px solid rgba(239, 68, 68, 0.2); }\n"
        "        h1 { font-size: 4rem; margin: 0; color: #ef4444; font-weight: 800; }\n"
        "        h2 { font-size: 1.5rem; margin: 0.5rem 0 1rem 0; color: #e2e8f0; }\n"
        "        p { color: #94a3b8; line-height: 1.6; margin-bottom: 2rem; }\n"
        "        .footer { font-size: 0.8rem; color: #64748b; border-top: 1px solid #334155; padding-top: 1rem; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"error-card\">\n"
        "        <h1>%d</h1>\n"
        "        <h2>%s</h2>\n"
        "        <p>%s</p>\n"
        "        <div class=\"footer\">LITH Server • Powered by FomaDev</div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>",
        status_code, title, status_code, title, message
    );
    
    return error_buffer;
}