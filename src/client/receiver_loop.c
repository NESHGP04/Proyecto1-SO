/*
 * src/client/receiver_loop.c
 * ─────────────────────────────────────────────────────────────────
 * Thread receptor del cliente.
 * Corre en paralelo al input_loop — escucha del servidor
 * continuamente mientras el usuario escribe.
 *
 * Mensajes que maneja:
 *   DELIVER          → mensaje privado entrante
 *   BROADCAST_FROM   → mensaje del chat general
 *   STATUS_CHANGED   → cambio de status de otro usuario
 *   LIST_OK          → respuesta a /list
 *   INFO_OK          → respuesta a /info
 *   STATUS_OK        → confirmación de cambio de status propio
 *   MSG_OK           → confirmación de mensaje privado enviado
 *   BROADCAST_OK     → confirmación de broadcast enviado
 *   ERROR            → error del servidor
 *   BYE              → servidor cerró la sesión → terminar
 *
 * Autora: Marinés García - 23391
 * CC3064 Sistemas Operativos — Proyecto 01
 * ─────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "../common/protocol.c"
#include "../../include/client.h"

/* ═══════════════════════════════════════════════════════════════
 * recv_line()
 * Lee del socket byte a byte hasta '\n' o hasta llenar el buffer.
 * Garantiza que buf termina en '\0'.
 *
 * Retorna:
 *   > 0  → bytes leídos (incluyendo el '\n')
 *   = 0  → el servidor cerró la conexión (EOF)
 *   < 0  → error de recv()
 * ═══════════════════════════════════════════════════════════════ */
int recv_line(int sockfd, char *buf, int max_len) {
    int n = 0;
    char c;

    while (n < max_len - 1) {
        int r = (int)recv(sockfd, &c, 1, 0);
        if (r == 0) { buf[n] = '\0'; return 0;  }
        if (r <  0) { buf[n] = '\0'; return -1; }
        buf[n++] = c;
        if (c == '\n') break;
    }
    buf[n] = '\0';
    return n;
}

/* ═══════════════════════════════════════════════════════════════
 * Helpers de parsing de campos
 * ═══════════════════════════════════════════════════════════════ */

/*
 * parse_two_fields()
 * rest = "f1|f2[|resto ignorado]"
 * Retorna 1 si OK, 0 si faltan campos.
 */
static int parse_two_fields(char *rest,
                             char *f1, int f1_len,
                             char *f2, int f2_len)
{
    if (!rest) return 0;
    char *tok = strtok(rest, FIELD_SEP);
    if (!tok) return 0;
    strncpy(f1, tok, f1_len - 1); f1[f1_len-1] = '\0';
    f1[strcspn(f1, "\n")] = '\0';

    tok = strtok(NULL, FIELD_SEP);
    if (!tok) return 0;
    strncpy(f2, tok, f2_len - 1); f2[f2_len-1] = '\0';
    f2[strcspn(f2, "\n")] = '\0';
    return 1;
}

/*
 * parse_three_fields()
 * rest = "f1|f2|f3[|resto ignorado]"
 * Retorna 1 si OK, 0 si faltan campos.
 */
static int parse_three_fields(char *rest,
                               char *f1, int f1_len,
                               char *f2, int f2_len,
                               char *f3, int f3_len)
{
    if (!parse_two_fields(rest, f1, f1_len, f2, f2_len)) return 0;
    char *tok = strtok(NULL, FIELD_SEP);
    if (!tok) return 0;
    strncpy(f3, tok, f3_len - 1); f3[f3_len-1] = '\0';
    f3[strcspn(f3, "\n")] = '\0';
    return 1;
}

/*
 * parse_msg_fields()
 * Caso especial para DELIVER y BROADCAST_FROM:
 * el mensaje puede contener '|' internos, así que NO usamos strtok
 * para el segundo campo — tomamos todo lo que queda después del
 * primer separador.
 *
 * rest = "origen|mensaje con | dentro"
 */
static void parse_msg_fields(char *rest,
                              char *from, int from_len,
                              char *msg,  int msg_len)
{
    from[0] = '\0';
    msg[0]  = '\0';
    if (!rest) return;

    char *sep = strchr(rest, FIELD_SEP_CHAR);
    if (sep) {
        size_t len = (size_t)(sep - rest);
        strncpy(from, rest, len < (size_t)from_len ? len : (size_t)(from_len-1));
        from[from_len-1] = '\0';
        from[strcspn(from, "\n")] = '\0';

        strncpy(msg, sep + 1, msg_len - 1);
        msg[msg_len-1] = '\0';
        msg[strcspn(msg, "\n")] = '\0';
    } else {
        strncpy(from, rest, from_len - 1);
        from[from_len-1] = '\0';
        from[strcspn(from, "\n")] = '\0';
    }
}

/* ═══════════════════════════════════════════════════════════════
 * dispatch_message()
 * Parsea una línea recibida y llama a la función de UI correcta.
 *
 * IMPORTANTE: siempre usa print_mutex alrededor de las llamadas
 * a ui_* para no mezclar output con el input_loop.
 *
 * Retorna:
 *    0  → seguir escuchando
 *   -1  → terminar el thread (BYE recibido)
 * ═══════════════════════════════════════════════════════════════ */
static int dispatch_message(ClientState *state, char *line) {
    char type[MAX_MSG_LEN];
    char rest_buf[MAX_MSG_LEN];

    strncpy(rest_buf, line, MAX_MSG_LEN - 1);
    rest_buf[MAX_MSG_LEN - 1] = '\0';

    char *rest = proto_parse_type(rest_buf, type);

    /* ── DELIVER|<origen>|<mensaje> ──────────────────────────── */
    if (strcmp(type, SRV_DELIVER) == 0) {
        char from[MAX_USERNAME_LEN] = "";
        char msg[MAX_CHAT_LEN]      = "";
        parse_msg_fields(rest, from, MAX_USERNAME_LEN, msg, MAX_CHAT_LEN);

        pthread_mutex_lock(&state->print_mutex);
        ui_print_private_in(state, from, msg);
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── BROADCAST_FROM|<origen>|<mensaje> ───────────────────── */
    if (strcmp(type, SRV_BROADCAST_FROM) == 0) {
        char from[MAX_USERNAME_LEN] = "";
        char msg[MAX_CHAT_LEN]      = "";
        parse_msg_fields(rest, from, MAX_USERNAME_LEN, msg, MAX_CHAT_LEN);

        pthread_mutex_lock(&state->print_mutex);
        ui_print_broadcast_in(state, from, msg);
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── STATUS_CHANGED|<username>|<status> ──────────────────── */
    if (strcmp(type, SRV_STATUS_CHANGED) == 0) {
        char username[MAX_USERNAME_LEN] = "";
        char new_status[16]             = "";
        parse_two_fields(rest, username, MAX_USERNAME_LEN, new_status, 16);

        pthread_mutex_lock(&state->print_mutex);
        ui_print_status_changed(state, username, new_status);
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── LIST_OK|<user1>;<user2>;... ─────────────────────────── */
    if (strcmp(type, SRV_LIST_OK) == 0) {
        char users[MAX_MSG_LEN] = "";
        if (rest) {
            strncpy(users, rest, MAX_MSG_LEN - 1);
            users[strcspn(users, "\n")] = '\0';
        }

        pthread_mutex_lock(&state->print_mutex);
        ui_print_list(users);
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── INFO_OK|<username>|<ip>|<status> ────────────────────── */
    if (strcmp(type, SRV_INFO_OK) == 0) {
        char username[MAX_USERNAME_LEN] = "";
        char ip[64]                     = "";
        char status[16]                 = "";
        parse_three_fields(rest,
                           username, MAX_USERNAME_LEN,
                           ip, 64,
                           status, 16);

        pthread_mutex_lock(&state->print_mutex);
        ui_print_info(username, ip, status);
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── STATUS_OK|<status> ───────────────────────────────────── */
    if (strcmp(type, SRV_STATUS_OK) == 0) {
        char new_status[16] = "";
        if (rest) {
            strncpy(new_status, rest, 15);
            new_status[strcspn(new_status, "\n")] = '\0';
        }

        pthread_mutex_lock(&state->print_mutex);
        ui_print_status_ok(state, new_status);
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── MSG_OK|<destino> ─────────────────────────────────────── */
    if (strcmp(type, SRV_MSG_OK) == 0) {
        char dest[MAX_USERNAME_LEN] = "";
        if (rest) {
            strncpy(dest, rest, MAX_USERNAME_LEN - 1);
            dest[strcspn(dest, "\n")] = '\0';
        }

        pthread_mutex_lock(&state->print_mutex);
        ui_print_msg_ok(dest);
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── BROADCAST_OK ─────────────────────────────────────────── */
    if (strcmp(type, SRV_BROADCAST_OK) == 0) {
        pthread_mutex_lock(&state->print_mutex);
        ui_print_broadcast_ok();
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── ERROR|<operacion>|<codigo>|<descripcion> ────────────── */
    if (strcmp(type, SRV_ERROR) == 0) {
        char operation[64]    = "";
        char code[64]         = "";
        char description[256] = "";

        if (rest) {
            char *tok = strtok(rest, FIELD_SEP);
            if (tok) { strncpy(operation, tok, 63); operation[strcspn(operation,"\n")] = '\0'; }
            tok = strtok(NULL, FIELD_SEP);
            if (tok) { strncpy(code, tok, 63);      code[strcspn(code,"\n")] = '\0'; }
            tok = strtok(NULL, FIELD_SEP);
            if (tok) { strncpy(description, tok, 255); description[strcspn(description,"\n")] = '\0'; }
        }

        pthread_mutex_lock(&state->print_mutex);
        ui_print_error(operation, code, description);
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return 0;
    }

    /* ── BYE ──────────────────────────────────────────────────── */
    if (strcmp(type, SRV_BYE) == 0) {
        pthread_mutex_lock(&state->print_mutex);
        ui_print_disconnected();
        pthread_mutex_unlock(&state->print_mutex);
        return -1;
    }

    /*
     * Mensaje desconocido — no crashear, solo loguear.
     * Útil durante integración cuando el servidor puede mandar
     * mensajes que el cliente aún no conoce.
     */
    pthread_mutex_lock(&state->print_mutex);
    fprintf(stderr, "[recv] Mensaje no reconocido: %s", line);
    ui_print_prompt();
    pthread_mutex_unlock(&state->print_mutex);
    return 0;
}

/* ═══════════════════════════════════════════════════════════════
 * receiver_loop()
 * Función del thread receptor. Firma compatible con pthread_create.
 * ═══════════════════════════════════════════════════════════════ */
void *receiver_loop(void *arg) {
    ClientState *state = (ClientState *)arg;
    char line[MAX_MSG_LEN];

    while (state->running) {
        int n = recv_line(state->sockfd, line, MAX_MSG_LEN);

        if (n == 0) {
            /* Servidor cerró la conexión sin mandar BYE (caída abrupta) */
            if (state->running) {
                pthread_mutex_lock(&state->print_mutex);
                ui_print_disconnected();
                pthread_mutex_unlock(&state->print_mutex);
            }
            break;
        }

        if (n < 0) {
            /* Error de red — solo salir si aún estábamos activos */
            if (state->running) {
                pthread_mutex_lock(&state->print_mutex);
                fprintf(stderr, "\n[error] Pérdida de conexión con el servidor.\n");
                pthread_mutex_unlock(&state->print_mutex);
            }
            break;
        }

        int result = dispatch_message(state, line);
        if (result == -1) break;
    }

    /* Avisar al input_loop que debe terminar */
    state->running = 0;

    return NULL;
}