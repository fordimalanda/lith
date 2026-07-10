# ==============================================================================
#  LITH Engine - Multi-platform Automated Build Pipeline (Universal Subsystem)
# ==============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -pthread -MMD -MP
INCLUDES = -Iinclude
TARGET_DIR = bin

# Sources communes incluant le nouveau module d'encapsulation SSL
SRCS = src/main.c \
       src/server.c \
       src/server_worker.c \
       src/http_parser.c \
       src/http_router.c \
       src/logger.c \
       src/server/config.c \
       src/server/utils.c \
       src/server/cache.c \
       src/server/ssl_engine.c

RES_OBJ = 

# Détection de l'OS et configuration dynamique des directives d'édition de liens
ifeq ($(OS),Windows_NT)
    TARGET = $(TARGET_DIR)/lith.exe
    LIBS = -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32
    MKDIR = mkdir -p $(TARGET_DIR)
    
    WINDRES = windres
    RES_OBJ = resources.res
else
    TARGET = $(TARGET_DIR)/lith
    LIBS = -lssl -lcrypto -pthread
    MKDIR = mkdir -p $(TARGET_DIR)
    
    SRCS += src/daemon.c
endif

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

all: $(TARGET)

$(TARGET): $(OBJS) $(RES_OBJ)
	@$(MKDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(RES_OBJ) $(LIBS)

%.res: %.rc
	$(WINDRES) $< -O coff -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEPS)

clean:
	-@rm -f src/*.o src/server/*.o src/*.d src/server/*.d *.res $(TARGET_DIR)/* 2>/dev/null || true
	-@rm -f src\*.o src\server\*.o src\*.d src\server\*.d *.res $(TARGET_DIR)\* 2>/dev/null || true

.PHONY: all clean