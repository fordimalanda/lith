#include "http_parser.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>

int parse_http_request(const char *raw_request, HttpRequest *out_request) {
    if (!raw_request || !out_request) return -1;

    // Copie locale pour ne pas modifier l'original pendant le découpage
    char buffer[2048];
    strncpy(buffer, raw_request, sizeof(buffer) - 1);

    // 1. Extraire la ligne de requête (ex: "GET /index.html HTTP/1.1")
    char *line = strtok(buffer, "\r\n");
    if (!line) return -1;

    char method[16], path[256];
    // sscanf est pratique ici pour découper par espaces
    if (sscanf(line, "%15s %255s", method, path) < 2) {
        return -1;
    }

    // 2. Identifier la méthode
    if (strcmp(method, "GET") == 0) {
        out_request->method = METHOD_GET;
    } else if (strcmp(method, "POST") == 0) {
        out_request->method = METHOD_POST;
    } else {
        out_request->method = METHOD_UNKNOWN;
    }

    // 3. Copier le chemin (Path)
    strncpy(out_request->path, path, MAX_PATH_SIZE - 1);

    // 4. Chercher le corps (Body) pour les requêtes POST
    // Le corps se trouve après la double ligne vide "\r\n\r\n"
    char *body_ptr = strstr(raw_request, "\r\n\r\n");
    if (body_ptr) {
        body_ptr += 4; // On avance de 4 caractères pour sauter les \r\n\r\n
        strncpy(out_request->body, body_ptr, MAX_BODY_SIZE - 1);
        out_request->content_length = strlen(out_request->body);
    } else {
        out_request->content_length = 0;
        out_request->body[0] = '\0';
    }

    return 0;
}

const char* method_to_str(HttpMethod method) {
    switch (method) {
        case METHOD_GET: return "GET";
        case METHOD_POST: return "POST";
        default: return "UNKNOWN";
    }
}