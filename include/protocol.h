//include/protocol.h
//define el lenguaje del sistema
//define la abstracción del protocolo para desacoplar parsing de la lógica del servidor.

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"

typedef enum { //convierte los strings en algo que es manejable en C
    CMD_REGISTER,
    CMD_LIST,
    CMD_INFO,
    CMD_STATUS,
    CMD_MSG,
    CMD_BROADCAST,
    CMD_QUIT,
    CMD_INVALID
} command_type_t;

typedef struct { //ayuda a representar MSG|juan|hola
    command_type_t type;
    char arg1[MAX_MSG];
    char arg2[MAX_MSG];
    char raw[MAX_MSG];
} parsed_message_t;

int parse_message(const char *line, parsed_message_t *out);

// builders
int build_register_ok(char *out, size_t n, const char *user, const char *ip);
int build_error(char *out, size_t n, const char *op, const char *code, const char *desc);
int build_list_ok(char *out, size_t n, const char *users);
int build_info_ok(char *out, size_t n, const char *user, const char *ip, const char *status);
int build_status_ok(char *out, size_t n, const char *status);
int build_msg_ok(char *out, size_t n, const char *dst);
int build_deliver(char *out, size_t n, const char *src, const char *msg);
int build_broadcast_ok(char *out, size_t n);
int build_broadcast_from(char *out, size_t n, const char *src, const char *msg);

#endif