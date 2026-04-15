/*
 * src/server/handlers.c
 * ─────────────────────────────────────────────────────────────────
 * Convierte mensajes parseados en acciones reales del servidor.
 * Flujo: mensaje llega → se parsea → handler ejecuta lógica.
 *
 * Bugs corregidos respecto a la versión original:
 *   1. session->fd       → session->sockfd  (nombre real del campo)
 *   2. "DUP"             → ERR_DUPLICATE_NAME
 *      "NOT_FOUND"       → ERR_USER_NOT_FOUND
 *   3. IP hardcodeada    → sockaddr_to_ip_string() desde session->addr
 *   4. handle_status     → ahora notifica STATUS_CHANGED a los demás
 *   5. handle_quit       → eliminado close(fd); lo maneja client_thread
 * ─────────────────────────────────────────────────────────────────
 */

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "../../include/parser.h"
#include "../../include/common.h"
#include "../../include/session.h"

/* ── Funciones externas del servidor (session_manager.c) ─────── */
extern int  username_exists(const char *user);
extern void add_session(const char *user, int fd);
extern int  find_user_fd(const char *user);
extern void broadcast_all(const char *msg);
extern void broadcast_except(const char *msg, int exclude_fd); /* nuevo */
extern void get_all_users(char *out, size_t n);
extern int  get_user_info(const char *user, char *ip, char *status);
extern void set_user_status(const char *user, const char *status);

/* ═══════════════════════════════════════════════════════════════
 * handle_register()
 * Valida username, revisa duplicado, crea sesión, responde.
 * ═══════════════════════════════════════════════════════════════ */
void handle_register(client_session_t *session, parsed_message_t *msg) {
    char buf[MAX_MSG];
    int  fd = session->sockfd;   /* BUG 1 corregido: era session->fd */

    if (!is_valid_username(msg->arg1)) {
        build_error(buf, sizeof(buf), CMD_REGISTER,
                    ERR_BAD_FORMAT, "Nombre invalido");   /* BUG 2 corregido */
        send(fd, buf, strlen(buf), MSG_NOSIGNAL);
        return;
    }

    if (username_exists(msg->arg1)) {
        build_error(buf, sizeof(buf), CMD_REGISTER,
                    ERR_DUPLICATE_NAME, "El nombre ya esta en uso"); /* BUG 2 */
        send(fd, buf, strlen(buf), MSG_NOSIGNAL);
        return;
    }

    add_session(msg->arg1, fd);

    /* BUG 3 corregido: IP real desde session->addr, no "0.0.0.0" */
    char ip[64] = "0.0.0.0";
    sockaddr_to_ip_string(&session->addr, ip, sizeof(ip));

    build_register_ok(buf, sizeof(buf), msg->arg1, ip);
    send(fd, buf, strlen(buf), MSG_NOSIGNAL);
}

/* ═══════════════════════════════════════════════════════════════
 * handle_msg()
 * Busca destino, entrega DELIVER, responde MSG_OK.
 * ═══════════════════════════════════════════════════════════════ */
void handle_msg(client_session_t *session, parsed_message_t *msg) {
    char buf[MAX_MSG];
    int  fd     = session->sockfd;
    const char *sender = session->username;
    int  dst_fd = find_user_fd(msg->arg1);

    if (dst_fd < 0) {
        build_error(buf, sizeof(buf), CMD_MSG,
                    ERR_USER_NOT_FOUND, "Usuario no existe"); /* BUG 2 */
        send(fd, buf, strlen(buf), MSG_NOSIGNAL);
        return;
    }

    build_deliver(buf, sizeof(buf), sender, msg->arg2);
    send(dst_fd, buf, strlen(buf), MSG_NOSIGNAL);

    build_msg_ok(buf, sizeof(buf), msg->arg1);
    send(fd, buf, strlen(buf), MSG_NOSIGNAL);
}

/* ═══════════════════════════════════════════════════════════════
 * handle_broadcast()
 * Envía a todos (incluye emisor), responde BROADCAST_OK.
 * ═══════════════════════════════════════════════════════════════ */
void handle_broadcast(client_session_t *session, parsed_message_t *msg) {
    char buf[MAX_MSG];
    int  fd = session->sockfd;

    build_broadcast_from(buf, sizeof(buf), session->username, msg->arg2);
    broadcast_all(buf);

    build_broadcast_ok(buf, sizeof(buf));
    send(fd, buf, strlen(buf), MSG_NOSIGNAL);
}

/* ═══════════════════════════════════════════════════════════════
 * handle_quit()
 * Manda BYE y retorna — NO cierra el fd aquí.
 * client_thread.c es responsable de cerrar y limpiar la sesión.
 * ═══════════════════════════════════════════════════════════════ */
void handle_quit(client_session_t *session) {
    char buf[64];
    /* BUG 5 corregido: eliminado close(fd) — lo maneja client_thread */
    snprintf(buf, sizeof(buf), "%s\n", SRV_BYE);
    send(session->sockfd, buf, strlen(buf), MSG_NOSIGNAL);
}

/* ═══════════════════════════════════════════════════════════════
 * handle_list()
 * Obtiene usuarios conectados, responde LIST_OK.
 * ═══════════════════════════════════════════════════════════════ */
void handle_list(client_session_t *session) {
    char users[MAX_MSG];
    char buf[MAX_MSG];

    get_all_users(users, sizeof(users));
    build_list_ok(buf, sizeof(buf), users);
    send(session->sockfd, buf, strlen(buf), MSG_NOSIGNAL);
}

/* ═══════════════════════════════════════════════════════════════
 * handle_info()
 * Verifica existencia, obtiene IP + estado, responde INFO_OK.
 * ═══════════════════════════════════════════════════════════════ */
void handle_info(client_session_t *session, parsed_message_t *msg) {
    char buf[MAX_MSG];
    char ip[64];
    char status[32];
    int  fd = session->sockfd;

    if (!username_exists(msg->arg1)) {
        build_error(buf, sizeof(buf), CMD_INFO,
                    ERR_USER_NOT_FOUND, "Usuario no existe"); /* BUG 2 */
        send(fd, buf, strlen(buf), MSG_NOSIGNAL);
        return;
    }

    if (!get_user_info(msg->arg1, ip, status)) {
        build_error(buf, sizeof(buf), CMD_INFO,
                    ERR_UNKNOWN_CMD, "No se pudo obtener info");
        send(fd, buf, strlen(buf), MSG_NOSIGNAL);
        return;
    }

    build_info_ok(buf, sizeof(buf), msg->arg1, ip, status);
    send(fd, buf, strlen(buf), MSG_NOSIGNAL);
}

/* ═══════════════════════════════════════════════════════════════
 * handle_status()
 * Valida estado, actualiza sesión, responde STATUS_OK,
 * y notifica STATUS_CHANGED a todos los demás.
 * ═══════════════════════════════════════════════════════════════ */
void handle_status(client_session_t *session, parsed_message_t *msg) {
    char buf[MAX_MSG];
    int  fd = session->sockfd;

    if (!is_valid_status(msg->arg1)) {
        build_error(buf, sizeof(buf), CMD_STATUS,
                    ERR_INVALID_STATUS, "Estado invalido");
        send(fd, buf, strlen(buf), MSG_NOSIGNAL);
        return;
    }

    set_user_status(session->username, msg->arg1);

    /* Confirmar al emisor */
    build_status_ok(buf, sizeof(buf), msg->arg1);
    send(fd, buf, strlen(buf), MSG_NOSIGNAL);

    /* BUG 4 corregido: notificar STATUS_CHANGED a los demás */
    build_status_changed(buf, sizeof(buf), session->username, msg->arg1);
    broadcast_except(buf, fd);
}

/* ═══════════════════════════════════════════════════════════════
 * handle_parsed_message()
 * Dispatcher principal — llamado por client_thread.c.
 * Retorna 0 para seguir, 1 para cerrar la sesión (QUIT).
 * ═══════════════════════════════════════════════════════════════ */
int handle_parsed_message(client_session_t *session,
                           const parsed_message_t *msg)
{
    if (!session || !msg) return -1;

    switch (msg->type) {

        case CMD_TYPE_REGISTER:
            handle_register(session, (parsed_message_t *)msg);
            break;

        case CMD_TYPE_LIST:
            handle_list(session);
            break;

        case CMD_TYPE_INFO:
            handle_info(session, (parsed_message_t *)msg);
            break;

        case CMD_TYPE_STATUS:
            handle_status(session, (parsed_message_t *)msg);
            break;

        case CMD_TYPE_MSG:
            handle_msg(session, (parsed_message_t *)msg);
            break;

        case CMD_TYPE_BROADCAST:
            handle_broadcast(session, (parsed_message_t *)msg);
            break;

        case CMD_TYPE_QUIT:
            handle_quit(session);
            return 1;

        default: {
            char buf[MAX_MSG];
            build_error(buf, sizeof(buf), "UNKNOWN",
                        ERR_UNKNOWN_CMD, "Comando invalido");
            send(session->sockfd, buf, strlen(buf), MSG_NOSIGNAL);
        }
    }

    return 0;
}