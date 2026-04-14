
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string.h>

#include "session.h"

typedef enum command_type {
    CMD_INVALID = 0,
    CMD_REGISTER,
    CMD_LIST,
    CMD_INFO,
    CMD_STATUS,
    CMD_MSG,
    CMD_BROADCAST,
    CMD_QUIT
} command_type_t;

typedef struct parsed_message {
    command_type_t type;
    char arg1[MAX_LINE_LEN];
    char arg2[MAX_LINE_LEN];
    char raw[MAX_LINE_LEN];
} parsed_message_t;

static inline int protocol_parse(const char *line, parsed_message_t *out)
{
    if (out != NULL) {
        memset(out, 0, sizeof(*out));
        out->type = CMD_INVALID;
        if (line != NULL) {
            strncpy(out->raw, line, sizeof(out->raw) - 1U);
            out->raw[sizeof(out->raw) - 1U] = '\0';
        }
    }
    return -1;
}

static inline int handle_parsed_message(client_session_t *session,
                                        const parsed_message_t *msg)
{
    UNUSED(session);
    UNUSED(msg);
    return -1;
}

#endif
