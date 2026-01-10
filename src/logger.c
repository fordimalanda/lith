#include "logger.h"
#include "common.h"
#include <stdarg.h>

void lith_log(LogLevel level, const char *format, ...) {
    // 1. Récupération de l'heure actuelle
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", t);

    // 2. Sélection de la couleur et du préfixe selon le niveau
    const char *prefix;
    const char *color;
    
    switch (level) {
        case LOG_INFO:
            prefix = "INFO";
            color = CLR_INFO;
            break;
        case LOG_WARN:
            prefix = "WARN";
            color = CLR_WARN;
            break;
        case LOG_ERROR:
            prefix = "ERR ";
            color = CLR_ERROR;
            break;
        default:
            prefix = "LOG ";
            color = CLR_RESET;
            break;
    }

    // 3. Affichage de l'en-tête [HH:MM:SS] [NIVEAU]
    printf("[%s] %s[%s]%s ", time_str, color, prefix, CLR_RESET);

    // 4. Gestion des arguments variables (comme printf)
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    // 5. Retour à la ligne et vidage du buffer pour affichage immédiat
    printf("\n");
    fflush(stdout);
}