//src/common/protocol.c
//Encapsula el parsing y la construcción de mensajes para cumplir estrictamente el protocolo

#include "../../include/protocol.h"

//convierte el mensaje en la estructura definida
//Maneja escapes (\|, \n, \\)
//Separa por |
//Identifica el comando
int parse_message(const char *line, parsed_message_t *out) { 
    if (!line || !out) return -1;

    memset(out, 0, sizeof(*out));
    strncpy(out->raw, line, MAX_MSG - 1);

    char fields[3][MAX_MSG] = {0};
    int f = 0, c = 0;

    for (int i = 0; line[i] && line[i] != '\n'; i++) {
        if (line[i] == '\\') {
            i++;
            if (!line[i]) break;
            if (line[i] == 'n') fields[f][c++] = '\n';
            else fields[f][c++] = line[i];
        }
        else if (line[i] == '|') {
            fields[f][c] = '\0';
            f++; c = 0;
            if (f >= 3) break;
        }
        else {
            fields[f][c++] = line[i];
        }
    }
    fields[f][c] = '\0';

    if (strcmp(fields[0], "REGISTER") == 0) out->type = CMD_REGISTER;
    else if (strcmp(fields[0], "LIST") == 0) out->type = CMD_LIST;
    else if (strcmp(fields[0], "INFO") == 0) out->type = CMD_INFO;
    else if (strcmp(fields[0], "STATUS") == 0) out->type = CMD_STATUS;
    else if (strcmp(fields[0], "MSG") == 0) out->type = CMD_MSG;
    else if (strcmp(fields[0], "BROADCAST") == 0) out->type = CMD_BROADCAST;
    else if (strcmp(fields[0], "QUIT") == 0) out->type = CMD_QUIT;
    else out->type = CMD_INVALID;

    strncpy(out->arg1, fields[1], MAX_MSG - 1);
    strncpy(out->arg2, fields[2], MAX_MSG - 1);

    return 0;
}

int protocol_parse(const char *line, parsed_message_t *out) {
    if (parse_message(line, out) != 0)
        return -1;

    if (!validate_message(out))
        return -1;

    return 0;
}

//builders
//sirven para garantizar que TODAS las respuestas tengan el formato correcto y que terminen en \n
int build_register_ok(char *out, size_t n, const char *user, const char *ip) {
    return snprintf(out, n, "REGISTER_OK|%s|%s|ACTIVO\n", user, ip);
}

int build_error(char *out, size_t n, const char *op, const char *code, const char *desc) {
    return snprintf(out, n, "ERROR|%s|%s|%s\n", op, code, desc);
}

int build_list_ok(char *out, size_t n, const char *users) {
    return snprintf(out, n, "LIST_OK|%s\n", users);
}

int build_info_ok(char *out, size_t n, const char *user, const char *ip, const char *status) {
    return snprintf(out, n, "INFO_OK|%s|%s|%s\n", user, ip, status);
}

int build_status_ok(char *out, size_t n, const char *status) {
    return snprintf(out, n, "STATUS_OK|%s\n", status);
}

int build_msg_ok(char *out, size_t n, const char *dst) {
    return snprintf(out, n, "MSG_OK|%s\n", dst);
}

int build_deliver(char *out, size_t n, const char *src, const char *msg) {
    return snprintf(out, n, "DELIVER|%s|%s\n", src, msg);
}

int build_broadcast_ok(char *out, size_t n) {
    return snprintf(out, n, "BROADCAST_OK\n");
}

int build_broadcast_from(char *out, size_t n, const char *src, const char *msg) {
    return snprintf(out, n, "BROADCAST_FROM|%s|%s\n", src, msg);
}