/*
 * src/client/input_loop.c
 * ─────────────────────────────────────────────────────────────────
 * Bucle de entrada del cliente.
 * Lee comandos desde stdin, los parsea y los convierte al
 * formato de protocolo para enviarlos al servidor.
 *
 * Comandos soportados:
 *   /list                   → LIST\n
 *   /info <usuario>         → INFO|<usuario>\n
 *   /status <estado>        → STATUS|<ACTIVO|OCUPADO|INACTIVO>\n
 *   /msg <destino> <texto>  → MSG|<destino>|<texto>\n
 *   /all <texto>            → BROADCAST|<texto>\n
 *   /quit                   → QUIT\n  (+ termina el cliente)
 *   /help                   → local, no viaja al servidor
 *
 * Autora: Marinés García - 23391
 * CC3064 Sistemas Operativos — Proyecto 01
 * ─────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>

#include "../common/protocol.c"
#include "../../include/client.h"

/* ═══════════════════════════════════════════════════════════════
 * Helpers internos
 * ═══════════════════════════════════════════════════════════════ */

/*
 * str_toupper_copy()
 * Copia src en dst convirtiéndolo a mayúsculas.
 * dst debe tener al menos max_len bytes.
 */
static void str_toupper_copy(char *dst, const char *src, int max_len) {
    int i;
    for (i = 0; i < max_len - 1 && src[i]; i++)
        dst[i] = (char)toupper((unsigned char)src[i]);
    dst[i] = '\0';
}

/*
 * trim_newline()
 * Elimina '\n' y '\r' del final de un string in-place.
 */
static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
}

/*
 * skip_spaces()
 * Avanza el puntero hasta el primer caracter no-espacio.
 */
static const char *skip_spaces(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

/*
 * send_proto()
 * Envía un string de protocolo ya construido al servidor.
 * Usa print_mutex para no mezclar output si hay error.
 * Retorna CLIENT_OK o CLIENT_ERR.
 */
static ClientResult send_proto(ClientState *state, const char *buf) {
    if (send(state->sockfd, buf, strlen(buf), MSG_NOSIGNAL) < 0) {
        pthread_mutex_lock(&state->print_mutex);
        fprintf(stderr, "[error] No se pudo enviar al servidor.\n");
        pthread_mutex_unlock(&state->print_mutex);
        return CLIENT_ERR;
    }
    return CLIENT_OK;
}

/* ═══════════════════════════════════════════════════════════════
 * parse_command()
 * Convierte una línea cruda de stdin en un ParsedCommand.
 * Siempre llena `out` (CMD_TYPE_UNKNOWN si no reconoce).
 * ═══════════════════════════════════════════════════════════════ */
void parse_command(const char *line, ParsedCommand *out) {
    memset(out, 0, sizeof(*out));
    out->type = CMD_TYPE_UNKNOWN;

    const char *p = skip_spaces(line);

    /* Debe empezar con '/' */
    if (*p != '/') return;
    p++; /* saltar el '/' */

    /* Extraer la palabra del comando (hasta espacio o fin) */
    char cmd_word[32] = "";
    int  wi = 0;
    while (*p && *p != ' ' && *p != '\t' && wi < 31)
        cmd_word[wi++] = *p++;
    cmd_word[wi] = '\0';

    /* Avanzar al primer argumento */
    p = skip_spaces(p);

    /* ── /list ── */
    if (strcmp(cmd_word, "list") == 0) {
        out->type = CMD_TYPE_LIST;
        return;
    }

    /* ── /help ── */
    if (strcmp(cmd_word, "help") == 0) {
        out->type = CMD_TYPE_HELP;
        return;
    }

    /* ── /quit ── */
    if (strcmp(cmd_word, "quit") == 0) {
        out->type = CMD_TYPE_QUIT;
        return;
    }

    /* ── /info <usuario> ── */
    if (strcmp(cmd_word, "info") == 0) {
        if (*p == '\0') return;  /* falta argumento → UNKNOWN */
        out->type = CMD_TYPE_INFO;
        strncpy(out->arg1, p, MAX_USERNAME_LEN - 1);
        trim_newline(out->arg1);
        return;
    }

    /* ── /status <activo|ocupado|inactivo> ── */
    if (strcmp(cmd_word, "status") == 0) {
        if (*p == '\0') return;
        out->type = CMD_TYPE_STATUS;
        /* Normalizar a mayúsculas: "ocupado" → "OCUPADO" */
        str_toupper_copy(out->arg1, p, sizeof(out->arg1));
        trim_newline(out->arg1);
        return;
    }

    /* ── /msg <destino> <mensaje> ── */
    if (strcmp(cmd_word, "msg") == 0) {
        if (*p == '\0') return;

        /* arg1 = primer token (destino) */
        const char *dest_start = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        size_t dest_len = (size_t)(p - dest_start);
        if (dest_len >= MAX_USERNAME_LEN) dest_len = MAX_USERNAME_LEN - 1;
        strncpy(out->arg1, dest_start, dest_len);
        out->arg1[dest_len] = '\0';

        /* arg2 = resto (mensaje completo, puede tener espacios) */
        p = skip_spaces(p);
        if (*p == '\0') return;  /* falta mensaje → UNKNOWN */
        out->type = CMD_TYPE_MSG;
        strncpy(out->arg2, p, MAX_CHAT_LEN - 1);
        trim_newline(out->arg2);
        return;
    }

    /* ── /all <mensaje> ── */
    if (strcmp(cmd_word, "all") == 0) {
        if (*p == '\0') return;
        out->type = CMD_TYPE_ALL;
        strncpy(out->arg2, p, MAX_CHAT_LEN - 1);
        trim_newline(out->arg2);
        return;
    }

    /* Comando no reconocido → out->type queda UNKNOWN */
}

/* ═══════════════════════════════════════════════════════════════
 * command_to_proto()
 * Convierte un ParsedCommand a string de protocolo listo para send.
 * Escribe en buf (tamaño MAX_MSG_LEN).
 *
 * Retorna CLIENT_ERR si el comando no debe viajar al servidor
 * (HELP, UNKNOWN) o si faltan campos requeridos.
 * ═══════════════════════════════════════════════════════════════ */
ClientResult command_to_proto(const ParsedCommand *cmd, char *buf) {
    buf[0] = '\0';

    switch (cmd->type) {
        case CMD_TYPE_LIST:
            PROTO_LIST(buf);
            return CLIENT_OK;

        case CMD_TYPE_INFO:
            if (cmd->arg1[0] == '\0') return CLIENT_ERR;
            PROTO_INFO(buf, cmd->arg1);
            return CLIENT_OK;

        case CMD_TYPE_STATUS:
            if (cmd->arg1[0] == '\0') return CLIENT_ERR;
            /* Validar que el status sea uno de los tres permitidos */
            if (strcmp(cmd->arg1, STATUS_ACTIVE)   != 0 &&
                strcmp(cmd->arg1, STATUS_BUSY)     != 0 &&
                strcmp(cmd->arg1, STATUS_INACTIVE) != 0) {
                return CLIENT_ERR;
            }
            PROTO_STATUS(buf, cmd->arg1);
            return CLIENT_OK;

        case CMD_TYPE_MSG:
            if (cmd->arg1[0] == '\0' || cmd->arg2[0] == '\0') return CLIENT_ERR;
            PROTO_MSG(buf, cmd->arg1, cmd->arg2);
            return CLIENT_OK;

        case CMD_TYPE_ALL:
            if (cmd->arg2[0] == '\0') return CLIENT_ERR;
            PROTO_BROADCAST(buf, cmd->arg2);
            return CLIENT_OK;

        case CMD_TYPE_QUIT:
            PROTO_QUIT(buf);
            return CLIENT_OK;

        case CMD_TYPE_HELP:
        case CMD_TYPE_UNKNOWN:
        default:
            return CLIENT_ERR;
    }
}

/* ═══════════════════════════════════════════════════════════════
 * handle_command()
 * Ejecuta un ParsedCommand ya validado:
 * - Si es local (/help) → lo maneja aquí mismo
 * - Si es /quit        → envía QUIT y retorna CLIENT_QUIT
 * - Resto              → convierte a protocolo y envía
 *
 * Retorna CLIENT_OK, CLIENT_QUIT o CLIENT_ERR.
 * ═══════════════════════════════════════════════════════════════ */
static ClientResult handle_command(ClientState *state,
                                   const ParsedCommand *cmd)
{
    /* ── /help — completamente local ── */
    if (cmd->type == CMD_TYPE_HELP) {
        pthread_mutex_lock(&state->print_mutex);
        cmd_help();
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return CLIENT_OK;
    }

    /* ── Comando desconocido ── */
    if (cmd->type == CMD_TYPE_UNKNOWN) {
        pthread_mutex_lock(&state->print_mutex);
        fprintf(stdout,
            "[error] Comando no reconocido.\n"
            "        Escribe /help para ver los comandos disponibles.\n");
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return CLIENT_OK;
    }

    /* ── /status con valor inválido ── */
    if (cmd->type == CMD_TYPE_STATUS) {
        if (strcmp(cmd->arg1, STATUS_ACTIVE)   != 0 &&
            strcmp(cmd->arg1, STATUS_BUSY)     != 0 &&
            strcmp(cmd->arg1, STATUS_INACTIVE) != 0) {
            pthread_mutex_lock(&state->print_mutex);
            fprintf(stdout,
                "[error] Status inválido: '%s'\n"
                "        Opciones: activo | ocupado | inactivo\n",
                cmd->arg1);
            ui_print_prompt();
            pthread_mutex_unlock(&state->print_mutex);
            return CLIENT_OK;
        }
    }

    /* ── /msg sin destino o sin mensaje ── */
    if (cmd->type == CMD_TYPE_MSG &&
        (cmd->arg1[0] == '\0' || cmd->arg2[0] == '\0')) {
        pthread_mutex_lock(&state->print_mutex);
        fprintf(stdout,
            "[error] Uso: /msg <usuario> <mensaje>\n"
            "        Ejemplo: /msg cami hola!\n");
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return CLIENT_OK;
    }

    /* ── /all sin mensaje ── */
    if (cmd->type == CMD_TYPE_ALL && cmd->arg2[0] == '\0') {
        pthread_mutex_lock(&state->print_mutex);
        fprintf(stdout, "[error] Uso: /all <mensaje>\n");
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return CLIENT_OK;
    }

    /* ── Convertir a protocolo ── */
    char buf[MAX_MSG_LEN];
    if (command_to_proto(cmd, buf) != CLIENT_OK) {
        pthread_mutex_lock(&state->print_mutex);
        fprintf(stdout, "[error] No se pudo construir el mensaje.\n");
        ui_print_prompt();
        pthread_mutex_unlock(&state->print_mutex);
        return CLIENT_OK;
    }

    /* ── /quit: enviar y señalar salida ── */
    if (cmd->type == CMD_TYPE_QUIT) {
        send_proto(state, buf);   /* QUIT\n */
        state->running = 0;
        return CLIENT_QUIT;
    }

    /* ── Enviar al servidor ── */
    return send_proto(state, buf);
}

/* ═══════════════════════════════════════════════════════════════
 * input_loop()
 * Loop principal de entrada. Corre en el hilo main mientras
 * receiver_loop corre en su propio thread.
 *
 * Retorna CLIENT_QUIT cuando el usuario escribe /quit,
 * o CLIENT_DISCONN si el servidor cerró la conexión
 * (detectado por state->running == 0).
 * ═══════════════════════════════════════════════════════════════ */
ClientResult input_loop(ClientState *state) {
    char         line[MAX_MSG_LEN];
    ParsedCommand cmd;

    /* Dibujar el primer prompt */
    pthread_mutex_lock(&state->print_mutex);
    ui_print_prompt();
    pthread_mutex_unlock(&state->print_mutex);

    while (state->running) {

        /* fgets bloquea esperando stdin */
        if (!fgets(line, sizeof(line), stdin)) {
            /*
             * EOF en stdin (Ctrl+D) → tratar como /quit
             */
            state->running = 0;
            char buf[MAX_MSG_LEN];
            PROTO_QUIT(buf);
            send_proto(state, buf);
            return CLIENT_QUIT;
        }

        /*
         * Si el receiver_loop puso running=0 mientras
         * fgets estaba bloqueado, salimos sin enviar nada más.
         */
        if (!state->running) return CLIENT_DISCONN;

        /* Ignorar líneas vacías (solo Enter) */
        const char *trimmed = skip_spaces(line);
        if (*trimmed == '\n' || *trimmed == '\0') {
            pthread_mutex_lock(&state->print_mutex);
            ui_print_prompt();
            pthread_mutex_unlock(&state->print_mutex);
            continue;
        }

        /* Parsear y ejecutar */
        parse_command(line, &cmd);
        ClientResult result = handle_command(state, &cmd);

        if (result == CLIENT_QUIT)   return CLIENT_QUIT;
        if (result == CLIENT_ERR)    return CLIENT_ERR;
        /* CLIENT_OK → seguir */
    }

    return CLIENT_DISCONN;
}