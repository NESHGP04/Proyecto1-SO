/*
 * src/common/protocol.c
 * ─────────────────────────────────────────────────────────────────
 * Encapsula el parsing y la construcción de mensajes para cumplir
 * estrictamente el protocolo.
 *
 * Validadores (is_valid_username, is_valid_status) viven en
 * validation.c — no se definen aquí para evitar duplicate symbol.
 * ─────────────────────────────────────────────────────────────────
 */

#include <string.h>
#include "../../include/parser.h"

int parse_message(const char *line, parsed_message_t *out) {
    if (!line || !out) return -1;

    memset(out, 0, sizeof(*out));
    strncpy(out->raw, line, MAX_MSG - 1);

    char fields[3][MAX_MSG] = {{0}};
    int f = 0, c = 0;

    for (int i = 0; line[i] && line[i] != '\n'; i++) {
        if (line[i] == '\\') {
            i++;
            if (!line[i]) break;
            if (line[i] == 'n') fields[f][c++] = '\n';
            else                fields[f][c++] = line[i];
        }
        else if (line[i] == '|') {
            fields[f][c] = '\0';
            f++; c = 0;
            if (f >= 3) break;
        }
        else {
            if (c < MAX_MSG - 1) fields[f][c++] = line[i];
        }
    }
    fields[f][c] = '\0';

    /*
     * Asignar el tipo usando los valores del ENUM (CMD_TYPE_*),
     * NO los string macros de protocol.h (CMD_REGISTER = "REGISTER").
     * Mezclarlos causa: "incompatible pointer to integer conversion".
     */
    if      (strcmp(fields[0], "REGISTER")  == 0) out->type = CMD_TYPE_REGISTER;
    else if (strcmp(fields[0], "LIST")      == 0) out->type = CMD_TYPE_LIST;
    else if (strcmp(fields[0], "INFO")      == 0) out->type = CMD_TYPE_INFO;
    else if (strcmp(fields[0], "STATUS")    == 0) out->type = CMD_TYPE_STATUS;
    else if (strcmp(fields[0], "MSG")       == 0) out->type = CMD_TYPE_MSG;
    else if (strcmp(fields[0], "BROADCAST") == 0) out->type = CMD_TYPE_BROADCAST;
    else if (strcmp(fields[0], "QUIT")      == 0) out->type = CMD_TYPE_QUIT;
    else                                           out->type = CMD_TYPE_INVALID;

    strncpy(out->arg1, fields[1], MAX_MSG - 1);
    strncpy(out->arg2, fields[2], MAX_MSG - 1);

    return (out->type == CMD_TYPE_INVALID) ? -1 : 0;
}

/* ── protocol_parse() ────────────────────────────────────────── */

int protocol_parse(const char *line, parsed_message_t *out) {
    return parse_message(line, out);
}

/* ── Builders ────────────────────────────────────────────────── */

int build_register_ok(char *out, size_t n, const char *user, const char *ip) {
    return snprintf(out, n, "REGISTER_OK|%s|%s|ACTIVO\n", user, ip);
}

int build_error(char *out, size_t n,
                const char *op, const char *code, const char *desc) {
    return snprintf(out, n, "ERROR|%s|%s|%s\n", op, code, desc);
}

int build_list_ok(char *out, size_t n, const char *users) {
    return snprintf(out, n, "LIST_OK|%s\n", users);
}

int build_info_ok(char *out, size_t n,
                  const char *user, const char *ip, const char *status) {
    return snprintf(out, n, "INFO_OK|%s|%s|%s\n", user, ip, status);
}

int build_status_ok(char *out, size_t n, const char *status) {
    return snprintf(out, n, "STATUS_OK|%s\n", status);
}

int build_status_changed(char *out, size_t n,
                         const char *user, const char *status) {
    return snprintf(out, n, "STATUS_CHANGED|%s|%s\n", user, status);
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

int build_broadcast_from(char *out, size_t n,
                         const char *src, const char *msg) {
    return snprintf(out, n, "BROADCAST_FROM|%s|%s\n", src, msg);
}