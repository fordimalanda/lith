CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
INCLUDES = -Iinclude
TARGET = bin/lith

# Détection de l'OS
ifeq ($(OS),Windows_NT)
    # Options spécifiques à Windows (MinGW)
    LIBS = -lws2_32
    TARGET := $(TARGET).exe
    MKDIR = if not exist bin mkdir bin
else
    # Options pour Linux/macOS/WSL
    LIBS = 
    MKDIR = mkdir -p bin
endif

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	@$(MKDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)