CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -pthread -Iinclude

SERVER_SOURCES = \
	src/common/utils.c \
	src/server/main_server.c \
	src/server/accept_loop.c \
	src/server/client_thread.c \
	src/server/session_manager.c \
	src/server/inactivity.c

SERVER_TARGET = server

.PHONY: all server clean

all: server

server: $(SERVER_SOURCES)
	$(CC) $(CFLAGS) $(SERVER_SOURCES) -o $(SERVER_TARGET)

clean:
	rm -f $(SERVER_TARGET)
