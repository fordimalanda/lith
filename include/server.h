#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#define DEFAULT_PORT 8080
#define BACKLOG 10          // Nombre maximum de connexions en attente
#define BUFFER_SIZE 2048    // Taille du tampon de réception

/**
 * @brief Structure pour passer les données nécessaires au thread de gestion client
 */
typedef struct {
    int client_socket;
    struct sockaddr_in client_address;
} ClientContext;

/**
 * @brief Initialise le socket serveur, lie (bind) et écoute (listen)
 * @param port Le port sur lequel le serveur doit écouter
 * @return Le descripteur de fichier du socket (fd), ou -1 en cas d'erreur
 */
int lith_init_server(int port);

/**
 * @brief Lance la boucle infinie d'acceptation des connexions
 * @param server_fd Le socket initialisé du serveur
 */
void lith_start_server(int server_fd);

/**
 * @brief Fonction exécutée par chaque thread pour traiter une requête
 * @param arg Pointeur vers un ClientContext
 */
void *lith_client_handler(void *arg);

#endif