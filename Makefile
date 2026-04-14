# ─────────────────────────────────────────────────────────────────
# Makefile — Chat Multithread
# CC3064 Sistemas Operativos — Proyecto 01
#
# Uso:
#   make              → compila client y stub_server
#   make server       → servidor real (cuando Nery/Cami terminen)
#   make client       → solo el cliente
#   make stub         → solo el stub server (pruebas)
#   make clean        → elimina binarios
#   make help         → muestra esta ayuda
# ─────────────────────────────────────────────────────────────────

CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -Iinclude
LFLAGS = -lpthread

# macOS: silenciar warnings de POSIX deprecados
UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
    CFLAGS += -Wno-deprecated-declarations
endif

# ── Fuentes ───────────────────────────────────────────────────────

# Común — agregar protocol.c y utils.c aquí cuando Cami los tenga
SRCS_COMMON =

# Servidor — Nery + Cami (descomentar cuando existan los archivos)
SRCS_SERVER = \
    src/server/main_server.c     \
    src/server/accept_loop.c     \
    src/server/client_thread.c   \
    src/server/session_manager.c \
    src/server/handlers.c        \
    src/server/inactivity.c

# Cliente — Marinés
SRCS_CLIENT = \
    src/client/main_client.c    \
    src/client/input_loop.c     \
    src/client/receiver_loop.c  \
    src/client/commands_local.c \
    src/client/ui.c

# ── Targets ───────────────────────────────────────────────────────
.PHONY: all client server stub clean help

all: client stub
	@echo ""
	@echo "  Listo. Binarios disponibles:"
	@echo "    ./client       — cliente de chat"
	@echo "    ./stub_server  — servidor de prueba"
	@echo "  Cuando el servidor esté listo: make server"
	@echo ""

client: $(SRCS_CLIENT)
	$(CC) $(CFLAGS) $(SRCS_COMMON) $(SRCS_CLIENT) -o client $(LFLAGS)
	@echo "  [ok] client compilado"

stub: tests/stub_server.c
	$(CC) $(CFLAGS) tests/stub_server.c -o stub_server $(LFLAGS)
	@echo "  [ok] stub_server compilado"

# Descomentar cuando main_server.c exista:
# server: $(SRCS_SERVER)
# 	$(CC) $(CFLAGS) $(SRCS_COMMON) $(SRCS_SERVER) -o server $(LFLAGS)
# 	@echo "  [ok] server compilado"
server:
	@echo "  [pendiente] Agregar src/server/main_server.c primero."
	@echo "              Luego descomentar el target 'server' en el Makefile."

clean:
	rm -f client server stub_server
	@echo "  [ok] binarios eliminados"

help:
	@echo ""
	@echo "  Chat Multithread — CC3064 SO — Proyecto 01"
	@echo ""
	@echo "  make              compila client y stub_server"
	@echo "  make client       solo el cliente"
	@echo "  make stub         solo el stub server"
	@echo "  make server       servidor real (pendiente)"
	@echo "  make clean        elimina binarios"
	@echo ""
	@echo "  Flujo de desarrollo (sin servidor real):"
	@echo "    Terminal 1:  ./stub_server 8080"
	@echo "    Terminal 2:  ./client marines 127.0.0.1 8080"
	@echo "    Terminal 3:  ./client cami 127.0.0.1 8080"
	@echo ""
	@echo "  Demo final (LAN con switch):"
	@echo "    Servidor:    ./server 8080"
	@echo "    Cliente:     ./client <usuario> <IP_servidor> 8080"
	@echo ""