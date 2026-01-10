CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
INCLUDES = -Iinclude
TARGET_DIR = bin
TARGET = $(TARGET_DIR)/lith

# Détection de l'OS pour les bibliothèques
ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
    TARGET := $(TARGET).exe
else
    LIBS = 
endif

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# On retire la commande de création de dossier automatique qui pose problème
# Créez simplement le dossier 'bin' à la main une seule fois
clean:
	rm -f src/*.o $(TARGET)