#ifndef SESSION_H
#define SESSION_H

#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#include "common.h"

typedef enum user_status {
    ST_ACTIVO = 0,
    ST_OCUPADO,
    ST_INACTIVO
} user_status_t;

typedef struct client_session {
    int sockfd;
    struct sockaddr_in addr;
    pthread_t thread_id;
    char username[MAX_USERNAME_LEN + 1];
    user_status_t status;
    time_t last_activity;
    int is_registered;
    int is_active;
} client_session_t;

const char *status_to_string(user_status_t status);

#endif
