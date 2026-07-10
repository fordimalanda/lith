# * Copyright (c) 2026 Fordi / FomaDev.
# * Licensed under FomaDev Public License.
# * See LICENSE file in the project root for full license information.

# Configuration générale des répertoires et compilateur
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -pthread -MMD -MP
INCLUDES = -Iinclude
TARGET_DIR = bin
SRC_DIR = src

# Liste de base des fichiers sources (Mise à jour du chemin de cache.c)
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/server_worker.c \
       $(SRC_DIR)/http_parser.c \
       $(SRC_DIR)/server/cache.c \
       $(SRC_DIR)/logger.c \
       $(SRC_DIR)/http_router.c \
       $(SRC_DIR)/server_utils.c

# Détection de l'OS pour adapter les dépendances, drapeaux et extensions
ifeq ($(OS),Windows_NT)
    TARGET = $(TARGET_DIR)/lith.exe
    
    # Si OPENSSL_ROOT est défini (ex: sur GitHub Actions avec /mingw64)
    ifneq ($(OPENSSL_ROOT),)
        INCLUDES += -I$(OPENSSL_ROOT)/include
        LIBS = -L$(OPENSSL_ROOT)/lib -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32
    else
        # Configuration locale par défaut sur ton PC (via ton dossier épuré deps/)
        INCLUDES += -I"deps/openssl/include"
        LIBS = -L"deps/openssl/lib/VC/x64/MD" -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32
    endif
else
    # Configuration Linux / Ubuntu standard
    TARGET = $(TARGET_DIR)/lith
    LIBS = -lssl -lcrypto -pthread
    SRCS += $(SRC_DIR)/daemon.c
endif

# Génération automatique des fichiers objets et des fichiers de dépendances
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

# Règle principale (Build par défaut)
all: $(TARGET)

# Liaison du binaire final
$(TARGET): $(OBJS)
	@mkdir -p $(TARGET_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LIBS)
	@echo "[+] Compilation successful : $(TARGET)"

# Règle générique pour la compilation des fichiers objets (.c -> .o)
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Inclusion des fichiers de dépendances générés automatiquement par GCC
-include $(DEPS)

# Nettoyage unifié et récursif des artefacts de compilation
clean:
	rm -f $(SRC_DIR)/*.o $(SRC_DIR)/*.d
	rm -f $(SRC_DIR)/server/*.o $(SRC_DIR)/server/*.d
	rm -f $(TARGET)
	@echo "[+] Project cleanup completed."

.PHONY: all clean