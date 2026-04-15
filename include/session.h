/*
 * include/session.h
 * ─────────────────────────────────────────────────────────────────
 * Estructura de sesión de cliente en el servidor.
 * Solo la usa el servidor — el cliente nunca incluye este archivo.
 *
 * Depende de: common.h
 * ─────────────────────────────────────────────────────────────────
 */

#ifndef SESSION_H
#define SESSION_H

#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#include "common.h"

/* Status del usuario — representación interna del servidor.
 * Para convertir a string del protocolo usar status_to_string(). */
typedef enum user_status {
    ST_ACTIVO   = 0,
    ST_OCUPADO,
    ST_INACTIVO
} user_status_t;

typedef struct client_session {
    int                sockfd;          /* fd del socket TCP                  */
    struct sockaddr_in addr;            /* dirección IP del cliente            */
    pthread_t          thread_id;       /* thread que maneja esta sesión       */
    char               username[MAX_USERNAME_LEN + 1];
    user_status_t      status;          /* ACTIVO / OCUPADO / INACTIVO         */
    time_t             last_activity;   /* timestamp del último mensaje        */
    int                is_registered;   /* 1 después de REGISTER_OK            */
    int                is_active;       /* 1 mientras la sesión está viva      */
} client_session_t;

/*
 * status_to_string()
 * Convierte user_status_t al string del protocolo.
 * Retorna "ACTIVO", "OCUPADO" o "INACTIVO".
 */
const char *status_to_string(user_status_t status);

#endif /* SESSION_H */