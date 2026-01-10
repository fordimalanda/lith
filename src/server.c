#include "server.h"
#include "common.h"
#include "logger.h"
#include "http_parser.h"
#include <pthread.h>

/**
 * Implémentation du Handler (Exécuté par chaque thread)
 */
void *lith_client_handler(void *arg) {
    ClientContext *ctx = (ClientContext *)arg;
    char buffer[BUFFER_SIZE] = {0};
    
    // 1. Lire les données envoyées par le client
    ssize_t valread = read(ctx->client_socket, buffer, BUFFER_SIZE - 1);
    
    if (valread > 0) {
        HttpRequest req;
        // 2. Analyser la requête avec notre parser
        if (parse_http_request(buffer, &req) == 0) {
            lith_log(LOG_INFO, "Requête : %s %s", method_to_str(req.method), req.path);

            // 3. Préparer une réponse HTTP basique
            // Dans une version future, nous pourrions servir de vrais fichiers ici
            char *response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html; charset=UTF-8\r\n"
                             "Server: LITH-Embedded\r\n"
                             "Connection: close\r\n"
                             "\r\n"
                             "<!DOCTYPE html>"
                             "<html><head><title>LITH Server</title></head>"
                             "<body style='font-family:sans-serif; text-align:center; padding-top:50px;'>"
                             "<h1>🚀 LITH est opérationnel</h1>"
                             "<p>Serveur HTTP minimaliste pour l'IoT et le test.</p>"
                             "</body></html>";
            
            send(ctx->client_socket, response, strlen(response), 0);
        } else {
            lith_log(LOG_WARN, "Requête malformée reçue.");
        }
    }

    // 4. Fermeture propre du socket et libération de la mémoire du contexte
    close(ctx->client_socket);
    free(ctx);
    return NULL;
}

/**
 * Initialisation du socket serveur
 */
int lith_init_server(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Création du point d'accès réseau
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_ERROR(server_fd < 0, "Échec de création du socket");

    // Permet de relancer le serveur sans attendre le timeout du noyau (SO_REUSEADDR)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        lith_log(LOG_WARN, "Configuration SO_REUSEADDR échouée");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Écoute sur toutes les interfaces réseau
    address.sin_port = htons(port);

    // Liaison du socket au port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        lith_log(LOG_ERROR, "Liaison au port %d impossible", port);
        return -1;
    }

    // Mise en mode écoute
    if (listen(server_fd, BACKLOG) < 0) {
        lith_log(LOG_ERROR, "Mise en mode écoute échouée");
        return -1;
    }

    lith_log(LOG_INFO, "LITH %s démarré sur le port %d", LITH_VERSION, port);
    return server_fd;
}

/**
 * Boucle principale d'acceptation
 */
void lith_start_server(int server_fd) {
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        // Allocation d'un contexte spécifique pour le thread à venir
        ClientContext *ctx = malloc(sizeof(ClientContext));
        if (!ctx) {
            lith_log(LOG_ERROR, "Mémoire insuffisante pour nouveau client");
            continue;
        }

        ctx->client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        ctx->client_address = client_addr;

        if (ctx->client_socket < 0) {
            lith_log(LOG_ERROR, "Erreur lors de l'acceptation");
            free(ctx);
            continue;
        }

        // Création du thread de traitement
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, lith_client_handler, (void *)ctx) != 0) {
            lith_log(LOG_ERROR, "Échec de création du thread");
            close(ctx->client_socket);
            free(ctx);
        } else {
            // Détachement pour éviter les fuites de ressources à la fin du thread
            pthread_detach(thread_id);
        }
    }
}