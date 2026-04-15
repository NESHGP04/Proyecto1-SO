/*
 * src/common/validation.c
 * ─────────────────────────────────────────────────────────────────
 * Valida que los mensajes sean correctos semánticamente.
 * Separa validación del parsing para evitar lógica duplicada
 * en handlers.
 * ─────────────────────────────────────────────────────────────────
 */

#include <ctype.h>    /* isalnum()              */
#include <string.h>   /* strlen(), strcmp()      */

#include "../../include/common.h"
#include "../../include/parser.h"

/* ── is_valid_username() ─────────────────────────────────────── */
int is_valid_username(const char *u) {
    if (!u) return 0;
    int len = (int)strlen(u);
    if (len == 0 || len > MAX_USER) return 0;

    for (int i = 0; i < len; i++) {
        if (!isalnum((unsigned char)u[i]) && u[i] != '_') return 0;
    }
    return 1;
}

/* ── is_valid_status() ───────────────────────────────────────── */
int is_valid_status(const char *s) {
    if (!s) return 0;
    return strcmp(s, STATUS_ACTIVE)   == 0 ||
           strcmp(s, STATUS_BUSY)     == 0 ||
           strcmp(s, STATUS_INACTIVE) == 0;
}

/* ── validate_message() ──────────────────────────────────────── */
int validate_message(parsed_message_t *msg) {
    if (!msg) return 0;

    switch (msg->type) {
        case CMD_TYPE_REGISTER:
            return is_valid_username(msg->arg1);

        case CMD_TYPE_STATUS:
            return is_valid_status(msg->arg1);

        case CMD_TYPE_MSG:
            return strlen(msg->arg1) > 0 && strlen(msg->arg2) > 0;

        default:
            return 1;
    }
}