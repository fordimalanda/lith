# * Copyright (c) 2026 Fordi / FomaDev. 
# * Licensed under FomaDev Public License.
# * See LICENSE file in the project root for full license information.

# Configuration générale des répertoires
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -pthread -MMD -MP
INCLUDES = -Iinclude
TARGET_DIR = bin
SRC_DIR = src

# Liste de base des fichiers sources (communs aux deux plateformes)
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/server_worker.c \
       $(SRC_DIR)/http_parser.c \
       $(SRC_DIR)/cache.c \
       $(SRC_DIR)/logger.c \
       $(SRC_DIR)/http_router.c \
       $(SRC_DIR)/server_utils.c

# Détection de l'OS et configuration dynamique des chemins d'édition de liens
ifeq ($(OS),Windows_NT)
    TARGET = $(TARGET_DIR)/lith.exe
    
    # [Windows MinGW standalone Configuration]
    MINGW_ROOT = D:/bin/gcc-15.2.0-gdb-16.3.90.20250511-binutils-2.45-mingw-w64-v13.0.0-ucrt
    
    # Ajout des deux chemins d'inclusion standard pour OpenSSL sous MinGW
    INCLUDES += -I"$(MINGW_ROOT)/include" -I"$(MINGW_ROOT)/x86_64-w64-mingw32/include"
    
    # Directives d'édition de liens pour OpenSSL et les couches réseaux natives Windows (WinSock2)
    LIBS = -L"$(MINGW_ROOT)/lib" -L"$(MINGW_ROOT)/x86_64-w64-mingw32/lib" -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32
    
    # Commandes système Windows
    MKDIR = if not exist $(TARGET_DIR) mkdir $(TARGET_DIR)
    RM = del /Q /F
    FIX_PATH = $(subst /,\\,$1)
else
    # [Linux / Ubuntu Configuration]
    TARGET = $(TARGET_DIR)/lith
    LIBS = -lssl -lcrypto -pthread
    SRCS += $(SRC_DIR)/daemon.c
    
    # Commandes système Unix
    MKDIR = mkdir -p $(TARGET_DIR)
    RM = rm -f
    FIX_PATH = $1
endif

# Génération automatique des fichiers objets et de dépendances
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

# Règle principale (Build par défaut)
all: $(TARGET)

# Liaison du binaire final
$(TARGET): $(OBJS)
	@$(MKDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LIBS)
	@echo "[+] Compilation réussie : $(TARGET)"

# Règle générique pour la compilation des fichiers objets (.c -> .o)
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Inclusion des fichiers de dépendances générés par GCC (-MMD -MP)
-include $(DEPS)

# Nettoyage des résidus de compilation et des binaires
clean:
ifeq ($(OS),Windows_NT)
	@if exist $(call FIX_PATH,$(SRC_DIR)/*.o) del /Q /F $(call FIX_PATH,$(SRC_DIR)/*.o)
	@if exist $(call FIX_PATH,$(SRC_DIR)/*.d) del /Q /F $(call FIX_PATH,$(SRC_DIR)/*.d)
	@if exist $(call FIX_PATH,$(TARGET)) del /Q /F $(call FIX_PATH,$(TARGET))
else
	$(RM) $(SRC_DIR)/*.o $(SRC_DIR)/*.d $(TARGET)
endif
	@echo "[+] Nettoyage du projet effectué."

.PHONY: all clean