//src/common/validation.c

#include "../../include/common.h"
#include "../../include/protocol.h"

int is_valid_username(const char *u) { //evalúa si el username es válido
    if (!u) return 0;
    int len = strlen(u);
    if (len == 0 || len > MAX_USER) return 0;

    for (int i = 0; i < len; i++) {
        if (!isalnum(u[i]) && u[i] != '_') return 0;
    }
    return 1;
}

int is_valid_status(const char *s) {
    return strcmp(s, "ACTIVO") == 0 ||
           strcmp(s, "OCUPADO") == 0 ||
           strcmp(s, "INACTIVO") == 0;
}

int validate_message(parsed_message_t *msg) { //valida el mensaje
    if (!msg) return 0;

    switch (msg->type) {
        case CMD_REGISTER:
            return is_valid_username(msg->arg1);

        case CMD_STATUS:
            return is_valid_status(msg->arg1);

        case CMD_MSG:
            return strlen(msg->arg1) > 0 && strlen(msg->arg2) > 0;

        default:
            return 1;
    }
}