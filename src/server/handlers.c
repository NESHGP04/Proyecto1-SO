//src/server/handlers.c
//convierte mensajes en acciones reales

#include "../../include/protocol.h"
#include "../../include/common.h"

//flujo real del servidor: llega mensaje, se parsea, se valida y el handler ejecuta lógica

extern int username_exists(const char *user);
extern void add_session(const char *user, int fd);
extern int find_user_fd(const char *user);
extern void broadcast_all(const char *msg);

//rechaza duplicados y valida formato
//valida username, revisa duplicado, crea sesión, responde
void handle_register(int fd, parsed_message_t *msg) {
    char buf[256];

    if (!is_valid_username(msg->arg1)) {
        build_error(buf, sizeof(buf), "REGISTER", "INVALID", "Nombre invalido");
        send(fd, buf, strlen(buf), 0);
        return;
    }

    if (username_exists(msg->arg1)) {
        build_error(buf, sizeof(buf), "REGISTER", "DUP", "Ya existe");
        send(fd, buf, strlen(buf), 0);
        return;
    }

    add_session(msg->arg1, fd);
    build_register_ok(buf, sizeof(buf), msg->arg1, "0.0.0.0");
    send(fd, buf, strlen(buf), 0);
}

//envía al destinatario y confirma al emisor
//busca destino, envía DELIVER, responde MSG_OK
void handle_msg(int fd, parsed_message_t *msg, const char *sender) {
    char buf[1024];
    int dst = find_user_fd(msg->arg1);

    if (dst < 0) {
        build_error(buf, sizeof(buf), "MSG", "NOT_FOUND", "Usuario no existe");
        send(fd, buf, strlen(buf), 0);
        return;
    }

    build_deliver(buf, sizeof(buf), sender, msg->arg2);
    send(dst, buf, strlen(buf), 0);

    build_msg_ok(buf, sizeof(buf), msg->arg1);
    send(fd, buf, strlen(buf), 0);
}

//envía a todos, incluye emisor, responde OK
void handle_broadcast(int fd, parsed_message_t *msg, const char *sender) {
    char buf[1024];

    build_broadcast_from(buf, sizeof(buf), sender, msg->arg2);
    broadcast_all(buf);

    build_broadcast_ok(buf, sizeof(buf));
    send(fd, buf, strlen(buf), 0);
}

//envía BYE, cierra conexión
void handle_quit(int fd) {
    char buf[64];
    snprintf(buf, sizeof(buf), "BYE\n");
    send(fd, buf, strlen(buf), 0);
    close(fd);
}

//obtiene los usuarios conectados, responde con LIST_OK|user1;user2
void handle_list(int fd) {
    char users[1024];
    char buf[1024];

    get_all_users(users, sizeof(users)); // "user1;user2;user3"
    build_list_ok(buf, sizeof(buf), users);
    send(fd, buf, strlen(buf), 0);
}

//verifica si el usuario existe, obtiene IP + estado y responde con INFO_OK
void handle_info(int fd, parsed_message_t *msg) {
    char buf[256];
    char ip[64];
    char status[32];

    if (!username_exists(msg->arg1)) {
        build_error(buf, sizeof(buf), "INFO", "NOT_FOUND", "Usuario no existe");
        send(fd, buf, strlen(buf), 0);
        return;
    }

    if (!get_user_info(msg->arg1, ip, status)) {
        build_error(buf, sizeof(buf), "INFO", "ERROR", "No se pudo obtener info");
        send(fd, buf, strlen(buf), 0);
        return;
    }

    build_info_ok(buf, sizeof(buf), msg->arg1, ip, status);
    send(fd, buf, strlen(buf), 0);
}

//valida estado (ACTIVO, OCUPADO, INACTIVO), actualiza en el sistema y responde STATUS_OK
void handle_status(int fd, parsed_message_t *msg, const char *user) {
    char buf[256];

    if (!is_valid_status(msg->arg1)) {
        build_error(buf, sizeof(buf), "STATUS", "INVALID", "Estado invalido");
        send(fd, buf, strlen(buf), 0);
        return;
    }

    set_user_status(user, msg->arg1);
    build_status_ok(buf, sizeof(buf), msg->arg1);
    send(fd, buf, strlen(buf), 0);
}
