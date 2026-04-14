/*
 * src/client/commands_local.c
 * ─────────────────────────────────────────────────────────────────
 * Comandos que se resuelven en el cliente sin viajar al servidor.
 * Por ahora solo /help, según la decisión de diseño del protocolo.
 *
 * Autora: Marinés García - 23391
 * CC3064 Sistemas Operativos — Proyecto 01
 * ─────────────────────────────────────────────────────────────────
 */

#include <stdio.h>
#include <unistd.h>
#include "../../include/client.h"

/* Reusar las mismas macros de color que ui.c sin duplicar la lógica */
static int colors_on(void) { return isatty(STDOUT_FILENO); }

#define C_RESET   (colors_on() ? "\033[0m"  : "")
#define C_BOLD    (colors_on() ? "\033[1m"  : "")
#define C_YELLOW  (colors_on() ? "\033[33m" : "")
#define C_BYELLOW (colors_on() ? "\033[93m" : "")
#define C_GRAY    (colors_on() ? "\033[90m" : "")
#define C_BCYAN   (colors_on() ? "\033[96m" : "")
#define C_BGREEN  (colors_on() ? "\033[92m" : "")
#define C_BBLUE   (colors_on() ? "\033[94m" : "")

/*
 * cmd_help()
 * Imprime todos los comandos disponibles con ejemplos.
 * No envía nada al servidor.
 */
void cmd_help(void) {
    printf("\n%s┌─ Comandos disponibles ────────────────────┐%s\n",
           C_BCYAN, C_RESET);

    printf("%s│%s  %-34s %s│%s\n", C_BCYAN, C_RESET, "", C_BCYAN, C_RESET);

    /* Macro para imprimir una fila */
    #define ROW(cmd, desc) \
        printf("%s│%s  %s%-12s%s %-22s %s│%s\n", \
               C_BCYAN, C_RESET, C_BYELLOW, cmd, C_RESET, desc, C_BCYAN, C_RESET)

    ROW("/list",              "Listar usuarios conectados");
    ROW("/info <usuario>",    "Info de un usuario");
    ROW("/status <estado>",   "Cambiar tu estado");
    ROW("/msg <user> <txt>",  "Mensaje privado");
    ROW("/all <mensaje>",     "Mensaje a todos");
    ROW("/quit",              "Desconectarse y salir");
    ROW("/help",              "Mostrar esta ayuda");

    #undef ROW

    printf("%s│%s  %-34s %s│%s\n", C_BCYAN, C_RESET, "", C_BCYAN, C_RESET);
    printf("%s└───────────────────────────────────────────┘%s\n",
           C_BCYAN, C_RESET);

    printf("\n  %sEstados válidos:%s  %sactivo%s  %socupado%s  %sinactivo%s\n\n",
           C_GRAY,   C_RESET,
           C_BGREEN, C_RESET,
           C_BYELLOW,C_RESET,
           C_GRAY,   C_RESET);

    printf("  %sEjemplos:%s\n", C_GRAY, C_RESET);
    printf("    %s/msg cami hola, cómo estás?%s\n",   C_BCYAN, C_RESET);
    printf("    %s/all buenas noches a todos%s\n",    C_BCYAN, C_RESET);
    printf("    %s/status ocupado%s\n",               C_BCYAN, C_RESET);
    printf("    %s/info nery%s\n\n",                  C_BCYAN, C_RESET);
}