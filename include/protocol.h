/*
 * include/protocol.h
 * ─────────────────────────────────────────────────────────────────
 * Contrato de protocolo compartido entre cliente y servidor.
 * CC3064 Sistemas Operativos — Proyecto 01: Chat Multithread
 *
 * REGLA: este archivo no incluye session.h ni client.h.
 *        Solo define el lenguaje del protocolo — strings, límites
 *        e inline helpers de construcción/parsing de mensajes.
 *
 * Depende de: common.h (para MAX_LINE_LEN y MAX_USERNAME_LEN)
 * ─────────────────────────────────────────────────────────────────
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <string.h>

#include "common.h"

/* ═══════════════════════════════════════════════════════════════
 * 1. VERSIÓN Y FORMATO
 * ═══════════════════════════════════════════════════════════════ */

#define PROTO_VERSION        "CHAT/1.0"

#define FIELD_SEP            "|"
#define FIELD_SEP_CHAR       '|'
#define MSG_TERMINATOR       "\n"
#define MSG_TERMINATOR_CHAR  '\n'

/* Longitud máxima del contenido de un mensaje de chat */
#define MAX_CHAT_LEN         900

/* ═══════════════════════════════════════════════════════════════
 * 2. COMANDOS CLIENTE → SERVIDOR
 * ═══════════════════════════════════════════════════════════════ */

#define CMD_REGISTER     "REGISTER"   /* REGISTER|<username>\n            */
#define CMD_LIST         "LIST"       /* LIST\n                           */
#define CMD_INFO         "INFO"       /* INFO|<username>\n                */
#define CMD_STATUS       "STATUS"     /* STATUS|<status>\n                */
#define CMD_MSG          "MSG"        /* MSG|<destino>|<mensaje>\n        */
#define CMD_BROADCAST    "BROADCAST"  /* BROADCAST|<mensaje>\n            */
#define CMD_QUIT         "QUIT"       /* QUIT\n                           */

/* ═══════════════════════════════════════════════════════════════
 * 3. RESPUESTAS SERVIDOR → CLIENTE
 * ═══════════════════════════════════════════════════════════════ */

#define SRV_WELCOME        "WELCOME"
#define SRV_REGISTER_OK    "REGISTER_OK"
#define SRV_LIST_OK        "LIST_OK"
#define SRV_INFO_OK        "INFO_OK"
#define SRV_STATUS_OK      "STATUS_OK"
#define SRV_MSG_OK         "MSG_OK"
#define SRV_DELIVER        "DELIVER"
#define SRV_BROADCAST_OK   "BROADCAST_OK"
#define SRV_BROADCAST_FROM "BROADCAST_FROM"
#define SRV_STATUS_CHANGED "STATUS_CHANGED"
#define SRV_ERROR          "ERROR"
#define SRV_BYE            "BYE"

/* ═══════════════════════════════════════════════════════════════
 * 4. VALORES DE STATUS
 * ═══════════════════════════════════════════════════════════════ */

#define STATUS_ACTIVE    "ACTIVO"
#define STATUS_BUSY      "OCUPADO"
#define STATUS_INACTIVE  "INACTIVO"

/* ═══════════════════════════════════════════════════════════════
 * 5. CÓDIGOS DE ERROR
 * ═══════════════════════════════════════════════════════════════ */

#define ERR_DUPLICATE_NAME  "DUPLICATE_NAME"
#define ERR_DUPLICATE_IP    "DUPLICATE_IP"
#define ERR_USER_NOT_FOUND  "USER_NOT_FOUND"
#define ERR_INVALID_STATUS  "INVALID_STATUS"
#define ERR_UNKNOWN_CMD     "UNKNOWN_CMD"
#define ERR_BAD_FORMAT      "BAD_FORMAT"

/* ═══════════════════════════════════════════════════════════════
 * 6. HELPERS INLINE DE CONSTRUCCIÓN Y PARSING
 * ═══════════════════════════════════════════════════════════════ */

/*
 * proto_build()
 * Construye "TIPO|f1|f2|f3\n" en buf.
 * Pasar NULL en los campos que no apliquen.
 */
static inline int proto_build(char *buf, size_t buf_size,
                               const char *type,
                               const char *f1,
                               const char *f2,
                               const char *f3)
{
    int n = 0;
    n += snprintf(buf + n, buf_size - (size_t)n, "%s", type);
    if (f1 && n < (int)buf_size)
        n += snprintf(buf + n, buf_size - (size_t)n, "|%s", f1);
    if (f2 && n < (int)buf_size)
        n += snprintf(buf + n, buf_size - (size_t)n, "|%s", f2);
    if (f3 && n < (int)buf_size)
        n += snprintf(buf + n, buf_size - (size_t)n, "|%s", f3);
    if (n < (int)buf_size)
        n += snprintf(buf + n, buf_size - (size_t)n, "\n");
    return n;
}

/*
 * proto_parse_type()
 * Extrae el tipo de una línea recibida. Modifica line in-place.
 * Retorna puntero al primer campo tras el tipo, o NULL.
 */
static inline char *proto_parse_type(char *line, char *type_out)
{
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';

    char *sep = strchr(line, FIELD_SEP_CHAR);
    if (!sep) {
        strncpy(type_out, line, MAX_MSG_LEN - 1);
        type_out[MAX_MSG_LEN - 1] = '\0';
        return NULL;
    }
    size_t type_len = (size_t)(sep - line);
    strncpy(type_out, line, type_len);
    type_out[type_len] = '\0';
    return sep + 1;
}

/* ── Macros de construcción rápida ───────────────────────────── */
#define PROTO_REGISTER(buf, user)      proto_build(buf,MAX_MSG_LEN,CMD_REGISTER, user,  NULL,NULL)
#define PROTO_LIST(buf)                proto_build(buf,MAX_MSG_LEN,CMD_LIST,     NULL,  NULL,NULL)
#define PROTO_INFO(buf, user)          proto_build(buf,MAX_MSG_LEN,CMD_INFO,     user,  NULL,NULL)
#define PROTO_STATUS(buf, status)      proto_build(buf,MAX_MSG_LEN,CMD_STATUS,   status,NULL,NULL)
#define PROTO_MSG(buf, dest, msg)      proto_build(buf,MAX_MSG_LEN,CMD_MSG,      dest,  msg, NULL)
#define PROTO_BROADCAST(buf, msg)      proto_build(buf,MAX_MSG_LEN,CMD_BROADCAST,msg,   NULL,NULL)
#define PROTO_QUIT(buf)                proto_build(buf,MAX_MSG_LEN,CMD_QUIT,     NULL,  NULL,NULL)

#endif /* PROTOCOL_H */