#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define MAX_METHOD_SIZE 8
#define MAX_PATH_SIZE 256
#define MAX_BODY_SIZE 2048

// Enumération pour faciliter le traitement des méthodes
typedef enum {
    METHOD_GET,
    METHOD_POST,
    METHOD_UNKNOWN
} HttpMethod;

// Structure principale pour stocker une requête analysée
typedef struct {
    HttpMethod method;
    char path[MAX_PATH_SIZE];
    char body[MAX_BODY_SIZE];
    int content_length;
} HttpRequest;

/**
 * @brief Analyse une chaîne brute (raw) reçue du socket
 * @param raw_request Le buffer brut reçu du client
 * @param out_request La structure à remplir
 * @return 0 si succès, -1 si erreur de format
 */
int parse_http_request(const char *raw_request, HttpRequest *out_request);

/**
 * @brief Utilitaire pour convertir l'enum Method en chaîne de caractères
 */
const char* method_to_str(HttpMethod method);

#endif