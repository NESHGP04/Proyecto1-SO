/*
 * src/client/main_client.c
 * ─────────────────────────────────────────────────────────────────
 * Punto de entrada del cliente de chat.
 * Responsabilidades:
 *   1. Leer argumentos CLI
 *   2. Crear socket y conectarse al servidor
 *   3. Completar el handshake (WELCOME → REGISTER → REGISTER_OK)
 *   4. Lanzar el thread receptor
 *   5. Entrar al input loop
 *   6. Desconectarse limpiamente al salir
 *
 * Uso:
 *   ./client <username> <IP_servidor> <puerto>
 *   Ejemplo: ./client marines 192.168.1.10 8080
 *
 * Autora: Marinés García - 23391
 * CC3064 Sistemas Operativos — Proyecto 01
 * ─────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#include "../../include/protocol.h"
#include "../../include/client.h"

/* ── Forward declarations de funciones de otros archivos ─────────
 * (definidas en input_loop.c, receiver_loop.c)                   */
ClientResult input_loop(ClientState *state);
void        *receiver_loop(void *arg);

/* ═══════════════════════════════════════════════════════════════
 * Validación de argumentos CLI
 * ═══════════════════════════════════════════════════════════════ */

/*
 * validate_username()
 * Verifica que el username solo contenga [A-Za-z0-9_]
 * y que no exceda MAX_USERNAME_LEN.
 * Retorna 1 si es válido, 0 si no.
 */
static int validate_username(const char *username) {
    if (!username || username[0] == '\0') return 0;

    int len = 0;
    for (const char *p = username; *p; p++, len++) {
        if (len >= MAX_USERNAME_LEN) return 0;
        char c = *p;
        int valid = (c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    (c == '_');
        if (!valid) return 0;
    }
    return len > 0;
}

/*
 * validate_port()
 * Verifica que el string sea un número entre 1 y 65535.
 * Retorna el puerto como int, o -1 si es inválido.
 */
static int validate_port(const char *port_str) {
    char *end;
    long port = strtol(port_str, &end, 10);
    if (*end != '\0' || port < 1 || port > 65535) return -1;
    return (int)port;
}

/* ═══════════════════════════════════════════════════════════════
 * client_connect()
 * ═══════════════════════════════════════════════════════════════ */

ClientResult client_connect(ClientState *state,
                             const char  *server_ip,
                             int          server_port)
{
    /* Crear socket TCP */
    state->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (state->sockfd < 0) {
        perror("[error] socket()");
        return CLIENT_ERR;
    }

    /* Configurar dirección del servidor */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "[error] IP inválida: %s\n", server_ip);
        close(state->sockfd);
        state->sockfd = -1;
        return CLIENT_ERR;
    }

    /* Conectar */
    if (connect(state->sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        fprintf(stderr, "[error] No se pudo conectar a %s:%d — %s\n",
                server_ip, server_port, strerror(errno));
        close(state->sockfd);
        state->sockfd = -1;
        return CLIENT_ERR;
    }

    /* Guardar info del servidor en el estado */
    strncpy(state->server_ip, server_ip, sizeof(state->server_ip) - 1);
    state->server_port = server_port;

    return CLIENT_OK;
}

/* ═══════════════════════════════════════════════════════════════
 * client_register()
 * ═══════════════════════════════════════════════════════════════ */

ClientResult client_register(ClientState *state, const char *username) {
    char buf[MAX_MSG_LEN];
    char type[MAX_MSG_LEN];

    /* ── Paso 1: esperar WELCOME|CHAT/1.0 ── */
    int n = recv_line(state->sockfd, buf, MAX_MSG_LEN);
    if (n <= 0) {
        fprintf(stderr, "[error] El servidor cerró la conexión antes del WELCOME.\n");
        return CLIENT_ERR;
    }

    char buf_copy[MAX_MSG_LEN];
    strncpy(buf_copy, buf, MAX_MSG_LEN - 1);
    buf_copy[MAX_MSG_LEN - 1] = '\0';

    char *rest = proto_parse_type(buf_copy, type);

    if (strcmp(type, SRV_WELCOME) != 0) {
        fprintf(stderr, "[error] Se esperaba WELCOME, se recibió: %s\n", buf);
        return CLIENT_ERR;
    }

    /* Verificar versión del protocolo */
    if (rest && strcmp(rest, PROTO_VERSION) != 0) {
        fprintf(stderr, "[advertencia] Versión de protocolo inesperada: %s\n", rest);
        /* No es fatal — continuamos */
    }

    /* ── Paso 2: enviar REGISTER|username ── */
    PROTO_REGISTER(buf, username);
    if (send(state->sockfd, buf, strlen(buf), 0) < 0) {
        perror("[error] send(REGISTER)");
        return CLIENT_ERR;
    }

    /* ── Paso 3: esperar REGISTER_OK o ERROR ── */
    n = recv_line(state->sockfd, buf, MAX_MSG_LEN);
    if (n <= 0) {
        fprintf(stderr, "[error] El servidor cerró la conexión durante el registro.\n");
        return CLIENT_ERR;
    }

    strncpy(buf_copy, buf, MAX_MSG_LEN - 1);
    buf_copy[MAX_MSG_LEN - 1] = '\0';
    rest = proto_parse_type(buf_copy, type);

    /* ── REGISTER_OK|<username>|<ip>|<status> ── */
    if (strcmp(type, SRV_REGISTER_OK) == 0) {
        /*
         * rest = "marines|192.168.1.11|ACTIVO"
         * Parsear los tres campos con strtok
         */
        if (rest) {
            char *tok = strtok(rest, FIELD_SEP);
            if (tok) strncpy(state->username, tok, MAX_USERNAME_LEN - 1);

            tok = strtok(NULL, FIELD_SEP);
            if (tok) strncpy(state->my_ip, tok, sizeof(state->my_ip) - 1);

            tok = strtok(NULL, FIELD_SEP);
            if (tok) {
                /* Quitar \n residual */
                tok[strcspn(tok, "\n")] = '\0';
                strncpy(state->my_status, tok, sizeof(state->my_status) - 1);
            }
        }
        return CLIENT_OK;
    }

    /* ── ERROR|REGISTER|<codigo>|<descripcion> ── */
    if (strcmp(type, SRV_ERROR) == 0) {
        /*
         * rest = "REGISTER|DUPLICATE_NAME|El nombre ya está en uso"
         * Mostrar el mensaje de error completo al usuario
         */
        char code[64]        = "DESCONOCIDO";
        char description[256] = "";

        if (rest) {
            char *tok = strtok(rest, FIELD_SEP);   /* operacion: "REGISTER" */
            tok = strtok(NULL, FIELD_SEP);          /* codigo               */
            if (tok) strncpy(code, tok, sizeof(code) - 1);
            tok = strtok(NULL, FIELD_SEP);          /* descripcion          */
            if (tok) {
                tok[strcspn(tok, "\n")] = '\0';
                strncpy(description, tok, sizeof(description) - 1);
            }
        }

        fprintf(stderr, "[error] Registro rechazado — %s: %s\n",
                code, description);
        return CLIENT_ERR;
    }

    /* Respuesta inesperada */
    fprintf(stderr, "[error] Respuesta inesperada del servidor: %s\n", buf);
    return CLIENT_ERR;
}

/* ═══════════════════════════════════════════════════════════════
 * client_run()
 * ═══════════════════════════════════════════════════════════════ */

ClientResult client_run(ClientState *state) {
    pthread_t recv_tid;

    /* Lanzar thread receptor */
    if (pthread_create(&recv_tid, NULL, receiver_loop, state) != 0) {
        perror("[error] pthread_create(receiver_loop)");
        return CLIENT_ERR;
    }

    /* Mostrar banner de bienvenida */
    ui_print_welcome(state);

    /* Entrar al input loop (bloquea hasta /quit o BYE) */
    ClientResult result = input_loop(state);

    /*
     * Cuando input_loop retorna, state->running ya es 0.
     * Esperamos a que el receiver_loop también termine.
     */
    pthread_join(recv_tid, NULL);

    return result;
}

/* ═══════════════════════════════════════════════════════════════
 * client_disconnect()
 * ═══════════════════════════════════════════════════════════════ */

void client_disconnect(ClientState *state) {
    if (state->sockfd < 0) return;

    /* Intentar enviar QUIT (puede fallar si el servidor ya cerró) */
    char buf[MAX_MSG_LEN];
    PROTO_QUIT(buf);
    send(state->sockfd, buf, strlen(buf), MSG_NOSIGNAL);

    close(state->sockfd);
    state->sockfd = -1;

    pthread_mutex_destroy(&state->print_mutex);
}

/* ═══════════════════════════════════════════════════════════════
 * main()
 * ═══════════════════════════════════════════════════════════════ */

int main(int argc, char *argv[]) {

    /* ── Argumentos CLI ── */
    if (argc != 4) {
        fprintf(stderr,
            "Uso: %s <username> <IP_servidor> <puerto>\n"
            "Ejemplo: %s marines 192.168.1.10 8080\n",
            argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *username   = argv[1];
    const char *server_ip  = argv[2];
    const char *port_str   = argv[3];

    /* Validar username */
    if (!validate_username(username)) {
        fprintf(stderr,
            "[error] Username inválido: '%s'\n"
            "        Solo se permiten letras, números y _ (máx %d caracteres)\n",
            username, MAX_USERNAME_LEN);
        return EXIT_FAILURE;
    }

    /* Validar puerto */
    int port = validate_port(port_str);
    if (port < 0) {
        fprintf(stderr,
            "[error] Puerto inválido: '%s'\n"
            "        Debe ser un número entre 1 y 65535\n",
            port_str);
        return EXIT_FAILURE;
    }

    /* ── Inicializar estado del cliente ── */
    ClientState state;
    memset(&state, 0, sizeof(state));
    state.sockfd  = -1;
    state.running = 1;
    strncpy(state.my_status, STATUS_ACTIVE, sizeof(state.my_status) - 1);

    if (pthread_mutex_init(&state.print_mutex, NULL) != 0) {
        perror("[error] pthread_mutex_init");
        return EXIT_FAILURE;
    }

    /* ── Conectar ── */
    fprintf(stdout, "Conectando a %s:%d...\n", server_ip, port);

    if (client_connect(&state, server_ip, port) != CLIENT_OK) {
        pthread_mutex_destroy(&state.print_mutex);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Conexión establecida.\n");

    /* ── Registrar ── */
    if (client_register(&state, username) != CLIENT_OK) {
        close(state.sockfd);
        pthread_mutex_destroy(&state.print_mutex);
        return EXIT_FAILURE;
    }

    /* ── Correr (input loop + receiver thread) ── */
    client_run(&state);

    /* ── Desconectar ── */
    client_disconnect(&state);

    fprintf(stdout, "\nHasta luego.\n");
    return EXIT_SUCCESS;
}