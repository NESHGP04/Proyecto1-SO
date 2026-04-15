/*
 * include/parser.h
 * ─────────────────────────────────────────────────────────────────
 * Tipos de parsing y builders de mensajes — solo del servidor.
 * El cliente no incluye este archivo.
 *
 * Depende de: protocol.h → common.h
 * ─────────────────────────────────────────────────────────────────
 */

#ifndef PARSER_H
#define PARSER_H

#include "protocol.h"

typedef enum command_type {
    CMD_TYPE_REGISTER  = 0,
    CMD_TYPE_LIST,
    CMD_TYPE_INFO,
    CMD_TYPE_STATUS,
    CMD_TYPE_MSG,
    CMD_TYPE_BROADCAST,
    CMD_TYPE_QUIT,
    CMD_TYPE_INVALID
} command_type_t;

typedef struct parsed_message {
    command_type_t type;
    char           arg1[MAX_MSG];
    char           arg2[MAX_MSG];
    char           raw[MAX_MSG];
} parsed_message_t;

int parse_message(const char *line, parsed_message_t *out);
int protocol_parse(const char *line, parsed_message_t *out);
int validate_message(parsed_message_t *msg);

int build_register_ok(char *out, size_t n, const char *user, const char *ip);
int build_error(char *out, size_t n, const char *op, const char *code, const char *desc);
int build_list_ok(char *out, size_t n, const char *users);
int build_info_ok(char *out, size_t n, const char *user, const char *ip, const char *status);
int build_status_ok(char *out, size_t n, const char *status);
int build_status_changed(char *out, size_t n, const char *user, const char *status);
int build_msg_ok(char *out, size_t n, const char *dst);
int build_deliver(char *out, size_t n, const char *src, const char *msg);
int build_broadcast_ok(char *out, size_t n);
int build_broadcast_from(char *out, size_t n, const char *src, const char *msg);

int is_valid_username(const char *user);
int is_valid_status(const char *status);

#endif /* PARSER_H */