# ==============================================================================
#  LITH Engine - Multi-platform Automated Build Pipeline (Universal Subsystem)
# ==============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -pthread -MMD -MP
INCLUDES = -Iinclude
TARGET_DIR = bin

# 1. Sources communes à toutes les plateformes (Ajout du module cache.c)
SRCS = src/main.c \
       src/server.c \
       src/server_worker.c \
       src/http_parser.c \
       src/http_router.c \
       src/logger.c \
       src/server/config.c \
       src/server/utils.c \
       src/server/cache.c

# Initialisation des variables spécifiques aux ressources Windows
RES_OBJ = 

# 2. Détection de l'OS et ajustement des dépendances / commandes systèmes
ifeq ($(OS),Windows_NT)
    TARGET = $(TARGET_DIR)/lith.exe
    LIBS = -lws2_32
    MKDIR = mkdir -p $(TARGET_DIR)
    
    # Compilation des ressources icones/méta pour l'exécutable Windows
    WINDRES = windres
    RES_OBJ = resources.res
else
    TARGET = $(TARGET_DIR)/lith
    LIBS = -pthread
    MKDIR = mkdir -p $(TARGET_DIR)
    
    # Inclusion exclusive du fichier démon uniquement sous UNIX
    SRCS += src/daemon.c
endif

# Déduction automatique des fichiers objets et dépendances (.d)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

# 3. Règle principale
all: $(TARGET)

# 4. Édition de liens (Génère l'exécutable final)
$(TARGET): $(OBJS) $(RES_OBJ)
	@$(MKDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(RES_OBJ) $(LIBS)

# 5. Règle de compilation des ressources Windows (.rc -> .res)
%.res: %.rc
	$(WINDRES) $< -O coff -o $@

# 6. Règle de compilation des fichiers objets (.c -> .o)
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 7. Inclusion des dépendances réelles générées dynamiquement par GCC
-include $(DEPS)

# 8. Nettoyage universel, robuste et multi-plateforme
clean:
	-@rm -f src/*.o src/server/*.o src/*.d src/server/*.d *.res $(TARGET_DIR)/* 2>/dev/null || true
	-@rm -f src\*.o src\server\*.o src\*.d src\server\*.d *.res $(TARGET_DIR)\* 2>/dev/null || true

.PHONY: all clean