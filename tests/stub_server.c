/*
 * tests/stub_server.c
 * ─────────────────────────────────────────────────────────────────
 * Servidor fake para desarrollo y prueba del cliente.
 * NO es el servidor real del proyecto — ese lo hace Nery.
 *
 * Qué hace:
 *   - Acepta hasta 3 conexiones TCP (una por hilo)
 *   - Manda WELCOME|CHAT/1.0 al conectarse
 *   - Responde mensajes hardcodeados que simulan el servidor real
 *   - Imprime en consola TODO lo que recibe (para verificar protocolo)
 *   - Permite probar el cliente 100% sin el servidor de Nery
 *
 * Uso:
 *   gcc -std=c11 -Wall -Iinclude tests/stub_server.c -lpthread -o stub_server
 *   ./stub_server 8080
 *
 * Autora: Marinés García - 23391
 * ─────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#include "../include/protocol.h"

/* ── Configuración ──────────────────────────────────────────────── */
#define MAX_STUB_CLIENTS  3
/* BACKLOG ya definido en common.h como 16 */

/* ── Colores para la consola del stub ──────────────────────────── */
#define COL_RESET   "\033[0m"
#define COL_RECV    "\033[36m"   /* cyan  — lo que llega del cliente  */
#define COL_SEND    "\033[33m"   /* amarillo — lo que enviamos        */
#define COL_INFO    "\033[32m"   /* verde  — eventos del stub         */
#define COL_WARN    "\033[31m"   /* rojo   — errores / desconocidos   */

/* ── Estado de cada cliente conectado al stub ───────────────────── */
typedef struct {
    int     sockfd;
    int     slot;                        /* índice 0-2                */
    char    username[MAX_USERNAME_LEN];  /* nombre registrado         */
    char    client_ip[64];              /* IP del cliente            */
} StubClient;

/* Tabla de clientes registrados (para /list, /info, STATUS_CHANGED) */
static StubClient g_clients[MAX_STUB_CLIENTS];
static int        g_count = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ── Helpers ────────────────────────────────────────────────────── */

/* Timestamp HH:MM:SS para los logs del stub */
static void ts(char *buf, int len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%H:%M:%S", tm);
}

/* Envía una línea al cliente y la imprime en consola */
static void stub_send(int sockfd, const char *msg) {
    char tsbuf[16];
    ts(tsbuf, sizeof(tsbuf));
    printf(COL_SEND "[%s] SEND → %s" COL_RESET, tsbuf, msg);
    fflush(stdout);
    send(sockfd, msg, strlen(msg), 0);
}

/* Lee una línea del socket (byte a byte hasta \n) */
static int stub_recv_line(int sockfd, char *buf, int maxlen) {
    int n = 0;
    char c;
    while (n < maxlen - 1) {
        int r = recv(sockfd, &c, 1, 0);
        if (r <= 0) return r;
        buf[n++] = c;
        if (c == '\n') break;
    }
    buf[n] = '\0';
    return n;
}

/* ── Lógica de respuestas ───────────────────────────────────────── */

/*
 * Procesa un mensaje ya recibido del cliente y genera la respuesta.
 * Retorna 0 para seguir, -1 para cerrar la conexión.
 */
static int handle_message(StubClient *cli, char *line) {
    char tsbuf[16];
    ts(tsbuf, sizeof(tsbuf));

    /* Imprimir lo recibido */
    printf(COL_RECV "[%s] RECV [%s] ← %s" COL_RESET,
           tsbuf,
           cli->username[0] ? cli->username : "?",
           line);
    fflush(stdout);

    /* Parsear tipo de mensaje */
    char line_copy[MAX_MSG_LEN];
    strncpy(line_copy, line, MAX_MSG_LEN - 1);
    line_copy[MAX_MSG_LEN - 1] = '\0';

    char type[MAX_MSG_LEN];
    char *rest = proto_parse_type(line_copy, type);

    char resp[MAX_MSG_LEN];

    /* ── REGISTER ─────────────────────────────────────────────── */
    if (strcmp(type, CMD_REGISTER) == 0) {

        if (!rest || rest[0] == '\0') {
            snprintf(resp, sizeof(resp),
                     "%s|%s|%s|campo username vacío\n",
                     SRV_ERROR, CMD_REGISTER, ERR_BAD_FORMAT);
            stub_send(cli->sockfd, resp);
            return -1;
        }

        /* Copiar username (rest apunta al campo) */
        char uname[MAX_USERNAME_LEN];
        strncpy(uname, rest, MAX_USERNAME_LEN - 1);
        uname[MAX_USERNAME_LEN - 1] = '\0';
        /* Quitar \n residual */
        uname[strcspn(uname, "\n")] = '\0';

        /* Verificar duplicado (simple, suficiente para stub) */
        pthread_mutex_lock(&g_mutex);
        int dup = 0;
        for (int i = 0; i < g_count; i++) {
            if (strcmp(g_clients[i].username, uname) == 0) { dup = 1; break; }
        }
        pthread_mutex_unlock(&g_mutex);

        if (dup) {
            snprintf(resp, sizeof(resp),
                     "%s|%s|%s|El nombre ya está en uso\n",
                     SRV_ERROR, CMD_REGISTER, ERR_DUPLICATE_NAME);
            stub_send(cli->sockfd, resp);
            return -1;
        }

        /* Registrar */
        strncpy(cli->username, uname, MAX_USERNAME_LEN - 1);
        pthread_mutex_lock(&g_mutex);
        g_clients[cli->slot] = *cli;
        g_count++;
        pthread_mutex_unlock(&g_mutex);

        snprintf(resp, sizeof(resp),
                 "%s|%s|%s|%s\n",
                 SRV_REGISTER_OK, uname, cli->client_ip, STATUS_ACTIVE);
        stub_send(cli->sockfd, resp);
        return 0;
    }

    /* ── LIST ─────────────────────────────────────────────────── */
    if (strcmp(type, CMD_LIST) == 0) {
        /* Construir "user1;user2;user3" */
        char users[MAX_MSG_LEN] = "";
        pthread_mutex_lock(&g_mutex);
        for (int i = 0; i < MAX_STUB_CLIENTS; i++) {
            if (g_clients[i].username[0]) {
                if (users[0]) strncat(users, ";", sizeof(users) - strlen(users) - 1);
                strncat(users, g_clients[i].username,
                        sizeof(users) - strlen(users) - 1);
            }
        }
        pthread_mutex_unlock(&g_mutex);

        snprintf(resp, sizeof(resp), "%s|%.900s\n", SRV_LIST_OK, users);
        stub_send(cli->sockfd, resp);
        return 0;
    }

    /* ── INFO ─────────────────────────────────────────────────── */
    if (strcmp(type, CMD_INFO) == 0) {
        char target[MAX_USERNAME_LEN] = "";
        if (rest) {
            strncpy(target, rest, MAX_USERNAME_LEN - 1);
            target[strcspn(target, "\n")] = '\0';
        }

        /* Buscar en tabla */
        pthread_mutex_lock(&g_mutex);
        int found = 0;
        for (int i = 0; i < MAX_STUB_CLIENTS; i++) {
            if (strcmp(g_clients[i].username, target) == 0) {
                snprintf(resp, sizeof(resp),
                         "%s|%s|%s|%s\n",
                         SRV_INFO_OK,
                         g_clients[i].username,
                         g_clients[i].client_ip,
                         STATUS_ACTIVE);
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&g_mutex);

        if (!found) {
            snprintf(resp, sizeof(resp),
                     "%s|%s|%s|Usuario no encontrado\n",
                     SRV_ERROR, CMD_INFO, ERR_USER_NOT_FOUND);
        }
        stub_send(cli->sockfd, resp);
        return 0;
    }

    /* ── STATUS ───────────────────────────────────────────────── */
    if (strcmp(type, CMD_STATUS) == 0) {
        char new_status[32] = "";
        if (rest) {
            strncpy(new_status, rest, 31);
            new_status[strcspn(new_status, "\n")] = '\0';
        }

        if (strcmp(new_status, STATUS_ACTIVE)   != 0 &&
            strcmp(new_status, STATUS_BUSY)     != 0 &&
            strcmp(new_status, STATUS_INACTIVE) != 0) {
            snprintf(resp, sizeof(resp),
                     "%s|%s|%s|Status inválido\n",
                     SRV_ERROR, CMD_STATUS, ERR_INVALID_STATUS);
        } else {
            snprintf(resp, sizeof(resp), "%s|%s\n", SRV_STATUS_OK, new_status);

            /* También mandar STATUS_CHANGED a los demás (simulado) */
            char notif[MAX_MSG_LEN];
            snprintf(notif, sizeof(notif),
                     "%s|%s|%s\n", SRV_STATUS_CHANGED, cli->username, new_status);
            pthread_mutex_lock(&g_mutex);
            for (int i = 0; i < MAX_STUB_CLIENTS; i++) {
                if (g_clients[i].username[0] &&
                    g_clients[i].sockfd != cli->sockfd) {
                    stub_send(g_clients[i].sockfd, notif);
                }
            }
            pthread_mutex_unlock(&g_mutex);
        }
        stub_send(cli->sockfd, resp);
        return 0;
    }

    /* ── MSG ──────────────────────────────────────────────────── */
    if (strcmp(type, CMD_MSG) == 0) {
        /* rest = "destino|mensaje" */
        char dest[MAX_USERNAME_LEN] = "";
        char msg[MAX_CHAT_LEN]      = "";

        if (rest) {
            char *sep = strchr(rest, FIELD_SEP_CHAR);
            if (sep) {
                size_t dlen = (size_t)(sep - rest);
                strncpy(dest, rest, dlen < MAX_USERNAME_LEN ? dlen : MAX_USERNAME_LEN - 1);
                strncpy(msg, sep + 1, MAX_CHAT_LEN - 1);
                msg[strcspn(msg, "\n")] = '\0';
            }
        }

        /* Buscar destino y entregar */
        pthread_mutex_lock(&g_mutex);
        int found = 0;
        for (int i = 0; i < MAX_STUB_CLIENTS; i++) {
            if (strcmp(g_clients[i].username, dest) == 0) {
                char deliver[MAX_MSG_LEN];
                snprintf(deliver, sizeof(deliver),
                         "%s|%s|%s\n", SRV_DELIVER, cli->username, msg);
                stub_send(g_clients[i].sockfd, deliver);
                found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&g_mutex);

        if (!found) {
            snprintf(resp, sizeof(resp),
                     "%s|%s|%s|Usuario no encontrado\n",
                     SRV_ERROR, CMD_MSG, ERR_USER_NOT_FOUND);
        } else {
            snprintf(resp, sizeof(resp), "%s|%s\n", SRV_MSG_OK, dest);
        }
        stub_send(cli->sockfd, resp);
        return 0;
    }

    /* ── BROADCAST ────────────────────────────────────────────── */
    if (strcmp(type, CMD_BROADCAST) == 0) {
        char msg[MAX_CHAT_LEN] = "";
        if (rest) {
            strncpy(msg, rest, MAX_CHAT_LEN - 1);
            msg[strcspn(msg, "\n")] = '\0';
        }

        char bcast[MAX_MSG_LEN];
        snprintf(bcast, sizeof(bcast),
                 "%s|%s|%s\n", SRV_BROADCAST_FROM, cli->username, msg);

        pthread_mutex_lock(&g_mutex);
        for (int i = 0; i < MAX_STUB_CLIENTS; i++) {
            if (g_clients[i].username[0]) {
                stub_send(g_clients[i].sockfd, bcast);
            }
        }
        pthread_mutex_unlock(&g_mutex);

        snprintf(resp, sizeof(resp), "%s\n", SRV_BROADCAST_OK);
        stub_send(cli->sockfd, resp);
        return 0;
    }

    /* ── QUIT ─────────────────────────────────────────────────── */
    if (strcmp(type, CMD_QUIT) == 0) {
        snprintf(resp, sizeof(resp), "%s\n", SRV_BYE);
        stub_send(cli->sockfd, resp);
        return -1;   /* señal para cerrar la conexión */
    }

    /* ── Comando desconocido ───────────────────────────────────── */
    printf(COL_WARN "[stub] Comando no reconocido: %s" COL_RESET, line);
    snprintf(resp, sizeof(resp),
             "%s|%.64s|%s|Comando desconocido\n",
             SRV_ERROR, type, ERR_UNKNOWN_CMD);
    stub_send(cli->sockfd, resp);
    return 0;
}

/* ── Thread por cliente ─────────────────────────────────────────── */
static void *client_thread(void *arg) {
    StubClient cli = *(StubClient *)arg;
    free(arg);

    char tsbuf[16];
    ts(tsbuf, sizeof(tsbuf));
    printf(COL_INFO "[%s] Nueva conexión desde %s (slot %d)\n" COL_RESET,
           tsbuf, cli.client_ip, cli.slot);

    /* Mandar WELCOME */
    char welcome[MAX_MSG_LEN];
    snprintf(welcome, sizeof(welcome), "%s|%s\n", SRV_WELCOME, PROTO_VERSION);
    stub_send(cli.sockfd, welcome);

    /* Loop de mensajes */
    char line[MAX_MSG_LEN];
    while (1) {
        int r = stub_recv_line(cli.sockfd, line, MAX_MSG_LEN);
        if (r <= 0) {
            ts(tsbuf, sizeof(tsbuf));
            printf(COL_WARN "[%s] Cliente [%s] cerró la conexión abruptamente.\n" COL_RESET,
                   tsbuf, cli.username[0] ? cli.username : "?");
            break;
        }
        int result = handle_message(&cli, line);
        if (result == -1) break;
    }

    /* Limpiar del registro global */
    pthread_mutex_lock(&g_mutex);
    if (cli.username[0]) {
        g_clients[cli.slot].username[0] = '\0';
        g_clients[cli.slot].sockfd = -1;
        g_count--;
        if (g_count < 0) g_count = 0;
    }
    pthread_mutex_unlock(&g_mutex);

    ts(tsbuf, sizeof(tsbuf));
    printf(COL_INFO "[%s] Sesión de [%s] cerrada. Clientes activos: %d\n" COL_RESET,
           tsbuf, cli.username[0] ? cli.username : "?", g_count);

    close(cli.sockfd);
    return NULL;
}

/* ── main ───────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 8080\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Puerto inválido: %s\n", argv[1]);
        return 1;
    }

    /* Inicializar tabla de clientes */
    memset(g_clients, 0, sizeof(g_clients));
    for (int i = 0; i < MAX_STUB_CLIENTS; i++) {
        g_clients[i].sockfd = -1;
    }

    /* Crear socket servidor */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    /* Reusar dirección para reiniciar rápido en desarrollo */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port),
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); return 1;
    }
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen"); close(server_fd); return 1;
    }

    printf(COL_INFO "╔══════════════════════════════════════╗\n");
    printf(         "║  STUB SERVER — Chat Multithread      ║\n");
    printf(         "║  Escuchando en puerto %-5d          ║\n", port);
    printf(         "║  Capacidad: %d clientes simultáneos  ║\n", MAX_STUB_CLIENTS);
    printf(         "╚══════════════════════════════════════╝\n" COL_RESET);
    fflush(stdout);

    /* Loop de aceptar conexiones */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_len);
        if (client_fd < 0) { perror("accept"); continue; }

        /* Buscar slot libre */
        pthread_mutex_lock(&g_mutex);
        int slot = -1;
        for (int i = 0; i < MAX_STUB_CLIENTS; i++) {
            if (g_clients[i].sockfd == -1) { slot = i; break; }
        }
        pthread_mutex_unlock(&g_mutex);

        if (slot == -1) {
            /* Servidor lleno */
            char full_msg[MAX_MSG_LEN];
            snprintf(full_msg, sizeof(full_msg),
                     "%s|%s|SERVER_FULL|El servidor está lleno\n",
                     SRV_ERROR, CMD_REGISTER);
            send(client_fd, full_msg, strlen(full_msg), 0);
            close(client_fd);
            printf(COL_WARN "[stub] Conexión rechazada: servidor lleno.\n" COL_RESET);
            continue;
        }

        /* Preparar info del cliente */
        StubClient *cli = malloc(sizeof(StubClient));
        memset(cli, 0, sizeof(StubClient));
        cli->sockfd = client_fd;
        cli->slot   = slot;
        inet_ntop(AF_INET, &client_addr.sin_addr,
                  cli->client_ip, sizeof(cli->client_ip));

        /* Reservar slot antes de lanzar el thread */
        pthread_mutex_lock(&g_mutex);
        g_clients[slot].sockfd = client_fd;
        strncpy(g_clients[slot].client_ip, cli->client_ip, 63);
        pthread_mutex_unlock(&g_mutex);

        /* Lanzar thread */
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cli);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}