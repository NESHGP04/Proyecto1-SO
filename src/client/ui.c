/*
 * src/client/ui.c
 * ─────────────────────────────────────────────────────────────────
 * Todas las funciones de salida a consola del cliente.
 * Centralizar el formato aquí significa que cambiar cómo se ve
 * el chat (colores, prefijos, estructura) solo requiere tocar
 * este archivo.
 *
 * Convención de prefijos visibles:
 *   >              prompt del usuario
 *   [sistema]      eventos de conexión / desconexión
 *   [ok]           confirmación de operación exitosa
 *   [error]        error del servidor o local
 *   >> privado      mensaje directo recibido
 *   ## broadcast    mensaje general recibido
 *   [~]            cambio de status (propio o ajeno)
 *   [usuarios]     lista de conectados
 *   [info]         información de un usuario
 *
 * Autora: Marinés García - 23391
 * CC3064 Sistemas Operativos — Proyecto 01
 * ─────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../../include/client.h"

/* ═══════════════════════════════════════════════════════════════
 * Colores ANSI
 * Detectamos si stdout es una terminal para no ensuciar logs
 * cuando la salida se redirige a un archivo.
 * ═══════════════════════════════════════════════════════════════ */
#include <unistd.h>

static int use_color = -1;  /* -1 = no inicializado */

static int colors_on(void) {
    if (use_color == -1)
        use_color = isatty(STDOUT_FILENO);
    return use_color;
}

#define C_RESET     (colors_on() ? "\033[0m"     : "")
#define C_BOLD      (colors_on() ? "\033[1m"     : "")
#define C_DIM       (colors_on() ? "\033[2m"     : "")

/* Colores de texto */
#define C_WHITE     (colors_on() ? "\033[97m"    : "")
#define C_CYAN      (colors_on() ? "\033[36m"    : "")
#define C_BCYAN     (colors_on() ? "\033[96m"    : "")
#define C_GREEN     (colors_on() ? "\033[32m"    : "")
#define C_BGREEN    (colors_on() ? "\033[92m"    : "")
#define C_YELLOW    (colors_on() ? "\033[33m"    : "")
#define C_BYELLOW   (colors_on() ? "\033[93m"    : "")
#define C_RED       (colors_on() ? "\033[31m"    : "")
#define C_MAGENTA   (colors_on() ? "\033[35m"    : "")
#define C_BMAGENTA  (colors_on() ? "\033[95m"    : "")
#define C_BLUE      (colors_on() ? "\033[34m"    : "")
#define C_BBLUE     (colors_on() ? "\033[94m"    : "")
#define C_GRAY      (colors_on() ? "\033[90m"    : "")

/* ═══════════════════════════════════════════════════════════════
 * Helpers internos
 * ═══════════════════════════════════════════════════════════════ */

/* Timestamp [HH:MM] para cada mensaje */
static void print_ts(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    printf("%s[%02d:%02d]%s ", C_GRAY, tm->tm_hour, tm->tm_min, C_RESET);
}

/* Separador de sección */
static void print_divider(void) {
    printf("%s────────────────────────────────────────%s\n", C_GRAY, C_RESET);
}

/* ═══════════════════════════════════════════════════════════════
 * ui_print_welcome()
 * Banner inicial que ve el usuario al conectarse y registrarse.
 * ═══════════════════════════════════════════════════════════════ */
void ui_print_welcome(const ClientState *state) {
    printf("\n");
    printf("%s╔══════════════════════════════════════════╗%s\n", C_BCYAN, C_RESET);
    printf("%s║%s  Chat Multithread  %sCCc3064 - UVG%s         %s║%s\n",
           C_BCYAN, C_RESET, C_DIM, C_RESET, C_BCYAN, C_RESET);
    printf("%s╚══════════════════════════════════════════╝%s\n", C_BCYAN, C_RESET);
    printf("  %sServidor%s : %s%s:%d%s\n",
           C_GRAY, C_RESET, C_WHITE, state->server_ip, state->server_port, C_RESET);
    printf("  %sUsuario%s  : %s%s%s\n",
           C_GRAY, C_RESET, C_BGREEN, state->username, C_RESET);
    printf("  %sIP propia%s: %s%s%s\n",
           C_GRAY, C_RESET, C_WHITE, state->my_ip, C_RESET);
    printf("  %sStatus%s   : %s%s%s\n",
           C_GRAY, C_RESET, C_BGREEN, state->my_status, C_RESET);
    printf("\n  Escribe %s/help%s para ver los comandos.\n\n",
           C_YELLOW, C_RESET);
    print_divider();
}

/* ═══════════════════════════════════════════════════════════════
 * ui_print_prompt()
 * Dibuja el prompt sin newline para que el usuario escriba
 * en la misma línea.
 * ═══════════════════════════════════════════════════════════════ */
void ui_print_prompt(void) {
    printf("%s>%s ", C_BYELLOW, C_RESET);
    fflush(stdout);
}

/* ═══════════════════════════════════════════════════════════════
 * Mensajes entrantes
 * ═══════════════════════════════════════════════════════════════ */

/*
 * ui_print_private_in()
 * Mensaje directo recibido.
 * Formato: [HH:MM] >> privado  cami → hola de regreso
 */
void ui_print_private_in(const ClientState *state,
                          const char *from, const char *msg)
{
    (void)state;
    printf("\n");
    print_ts();
    printf("%s>> privado%s  %s%s%s → %s\n",
           C_BMAGENTA, C_RESET,
           C_BOLD, from, C_RESET,
           msg);
}

/*
 * ui_print_broadcast_in()
 * Mensaje de chat general recibido.
 * Formato: [HH:MM] ## broadcast  nery → buenas noches
 */
void ui_print_broadcast_in(const ClientState *state,
                            const char *from, const char *msg)
{
    (void)state;
    printf("\n");
    print_ts();
    printf("%s## broadcast%s  %s%s%s → %s\n",
           C_BCYAN, C_RESET,
           C_BOLD, from, C_RESET,
           msg);
}

/*
 * ui_print_status_changed()
 * Notificación de cambio de status de cualquier usuario.
 * Formato: [HH:MM] [~] cami ahora está OCUPADO
 */
void ui_print_status_changed(const ClientState *state,
                              const char *user, const char *new_status)
{
    (void)state;

    /* Color según el status */
    const char *sc = C_BGREEN;
    if (strcmp(new_status, STATUS_BUSY)     == 0) sc = C_BYELLOW;
    if (strcmp(new_status, STATUS_INACTIVE) == 0) sc = C_GRAY;

    printf("\n");
    print_ts();
    printf("%s[~]%s %s%s%s ahora está %s%s%s\n",
           C_YELLOW, C_RESET,
           C_BOLD, user, C_RESET,
           sc, new_status, C_RESET);
}

/* ═══════════════════════════════════════════════════════════════
 * Confirmaciones de operaciones
 * ═══════════════════════════════════════════════════════════════ */

/* MSG_OK → [ok] mensaje enviado a cami */
void ui_print_msg_ok(const char *dest) {
    printf("%s[ok]%s mensaje enviado a %s%s%s\n",
           C_BGREEN, C_RESET, C_BOLD, dest, C_RESET);
}

/* BROADCAST_OK → [broadcast] enviado */
void ui_print_broadcast_ok(void) {
    printf("%s[broadcast]%s enviado\n", C_BCYAN, C_RESET);
}

/* STATUS_OK → [~] tu status es ahora OCUPADO
 * También actualiza state->my_status para que lo muestre /help. */
void ui_print_status_ok(ClientState *state, const char *status) {
    strncpy(state->my_status, status, sizeof(state->my_status) - 1);
    state->my_status[sizeof(state->my_status) - 1] = '\0';

    const char *sc = C_BGREEN;
    if (strcmp(status, STATUS_BUSY)     == 0) sc = C_BYELLOW;
    if (strcmp(status, STATUS_INACTIVE) == 0) sc = C_GRAY;

    printf("%s[~]%s tu status es ahora %s%s%s\n",
           C_YELLOW, C_RESET, sc, status, C_RESET);
}

/* ═══════════════════════════════════════════════════════════════
 * Respuestas a consultas
 * ═══════════════════════════════════════════════════════════════ */

/*
 * ui_print_list()
 * Muestra la lista de usuarios conectados.
 * users_raw = "cami;marines;nery"  (separados por ';')
 *
 * Ejemplo de salida:
 *   [usuarios] 3 conectados
 *     • cami
 *     • marines  ← tú
 *     • nery
 */
void ui_print_list(const char *users_raw) {
    /* Contar y listar */
    char buf[MAX_MSG_LEN];
    strncpy(buf, users_raw, MAX_MSG_LEN - 1);
    buf[MAX_MSG_LEN - 1] = '\0';

    /* Contar semicolons para saber cuántos hay */
    int count = 1;
    for (char *p = buf; *p; p++) if (*p == ';') count++;
    if (buf[0] == '\0') count = 0;

    printf("%s[usuarios]%s %s%d%s conectado%s\n",
           C_BBLUE, C_RESET,
           C_BOLD, count, C_RESET,
           count == 1 ? "" : "s");

    /* Volver a copiar para tokenizar */
    strncpy(buf, users_raw, MAX_MSG_LEN - 1);
    char *tok = strtok(buf, ";");
    while (tok) {
        tok[strcspn(tok, "\n")] = '\0';
        printf("  %s•%s %s\n", C_BBLUE, C_RESET, tok);
        tok = strtok(NULL, ";");
    }
}

/*
 * ui_print_info()
 * Muestra la información de un usuario específico.
 *
 * Ejemplo:
 *   [info] nery
 *     IP     : 192.168.1.12
 *     Status : ACTIVO
 */
void ui_print_info(const char *username,
                   const char *ip,
                   const char *status)
{
    const char *sc = C_BGREEN;
    if (strcmp(status, STATUS_BUSY)     == 0) sc = C_BYELLOW;
    if (strcmp(status, STATUS_INACTIVE) == 0) sc = C_GRAY;

    printf("%s[info]%s %s%s%s\n",
           C_BBLUE, C_RESET, C_BOLD, username, C_RESET);
    printf("  %sIP%s     : %s\n", C_GRAY, C_RESET, ip);
    printf("  %sStatus%s : %s%s%s\n", C_GRAY, C_RESET, sc, status, C_RESET);
}

/* ═══════════════════════════════════════════════════════════════
 * Errores y sistema
 * ═══════════════════════════════════════════════════════════════ */

/*
 * ui_print_error()
 * Muestra un error recibido del servidor.
 * Formato: [error] MSG/USER_NOT_FOUND: Usuario no encontrado
 */
void ui_print_error(const char *operation,
                    const char *code,
                    const char *description)
{
    printf("%s[error]%s ",  C_RED, C_RESET);

    if (operation[0] && code[0])
        printf("%s/%s: ", operation, code);
    else if (code[0])
        printf("%s: ", code);

    printf("%s\n", description[0] ? description : "(sin descripción)");
}

/*
 * ui_print_disconnected()
 * Mensaje cuando el servidor cierra la conexión.
 */
void ui_print_disconnected(void) {
    printf("\n");
    print_divider();
    printf("%s[sistema]%s Desconectado del servidor.\n",
           C_GRAY, C_RESET);
}