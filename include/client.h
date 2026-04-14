/*
 * client.h
 * ─────────────────────────────────────────────────────────────────
 * Declaraciones de tipos, estructuras y funciones del cliente.
 * CC3064 Sistemas Operativos — Proyecto 01: Chat Multithread
 *
 * Autora: Marinés García - 23391
 *
 * Archivos que implementan esto:
 *   src/client/main_client.c      — arranque, connect, registro
 *   src/client/input_loop.c       — lectura de stdin y conversión a protocolo
 *   src/client/receiver_loop.c    — thread que escucha mensajes del servidor
 *   src/client/commands_local.c   — comandos que no viajan al servidor (/help)
 *   src/client/ui.c               — formato de salida en consola
 * ─────────────────────────────────────────────────────────────────
 */

#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include "protocol.h"

/* ═══════════════════════════════════════════════════════════════
 * 1. ESTADO GLOBAL DEL CLIENTE
 *    Una sola instancia de esta struct vive en main_client.c
 *    y es compartida entre el input_loop y el receiver_loop.
 * ═══════════════════════════════════════════════════════════════ */

typedef struct {
    int     sockfd;                     /* File descriptor del socket TCP           */
    char    username[MAX_USERNAME_LEN]; /* Nombre de usuario registrado             */
    char    server_ip[64];             /* IP del servidor (para mostrar en UI)     */
    int     server_port;               /* Puerto del servidor                      */
    char    my_ip[64];                 /* IP propia (recibida en REGISTER_OK)      */
    char    my_status[16];             /* Status actual: ACTIVO/OCUPADO/INACTIVO   */
    int     running;                   /* Flag: 1 = cliente activo, 0 = salir      */
    pthread_mutex_t print_mutex;       /* Mutex para no mezclar salida en consola  */
} ClientState;

/* ═══════════════════════════════════════════════════════════════
 * 2. CÓDIGOS DE RESULTADO INTERNOS
 * ═══════════════════════════════════════════════════════════════ */

typedef enum {
    CLIENT_OK       =  0,   /* Operación exitosa                         */
    CLIENT_ERR      = -1,   /* Error genérico                            */
    CLIENT_QUIT     =  1,   /* El usuario pidió salir (/quit)            */
    CLIENT_DISCONN  =  2,   /* El servidor cerró la conexión (BYE)       */
} ClientResult;

/* ═══════════════════════════════════════════════════════════════
 * 3. TIPO INTERNO PARA COMANDOS DE CONSOLA
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Representa un comando ya parseado desde stdin.
 * input_loop.c lo construye; luego decide si es local o va al servidor.
 *
 * Ejemplo para "/msg cami hola como estás":
 *   cmd  = CMD_TYPE_MSG
 *   arg1 = "cami"
 *   arg2 = "hola como estás"
 */
typedef enum {
    CMD_TYPE_UNKNOWN = 0,
    CMD_TYPE_LIST,          /* /list                    */
    CMD_TYPE_INFO,          /* /info <usuario>          */
    CMD_TYPE_STATUS,        /* /status <activo|ocupado|inactivo> */
    CMD_TYPE_MSG,           /* /msg <destino> <mensaje> */
    CMD_TYPE_ALL,           /* /all <mensaje>           */
    CMD_TYPE_QUIT,          /* /quit                    */
    CMD_TYPE_HELP,          /* /help  — local, no va al servidor */
} CommandType;

typedef struct {
    CommandType type;
    char arg1[MAX_USERNAME_LEN];  /* usuario destino, status, etc. */
    char arg2[MAX_CHAT_LEN];      /* mensaje de chat               */
} ParsedCommand;

/* ═══════════════════════════════════════════════════════════════
 * 4. main_client.c
 * ═══════════════════════════════════════════════════════════════ */

/*
 * client_connect()
 * Crea el socket TCP y hace connect() hacia server_ip:server_port.
 * Si la conexión falla imprime el error y retorna CLIENT_ERR.
 * Si tiene éxito guarda el fd en state->sockfd y retorna CLIENT_OK.
 */
ClientResult client_connect(ClientState *state,
                             const char  *server_ip,
                             int          server_port);

/*
 * client_register()
 * Espera el mensaje WELCOME del servidor.
 * Luego pide el username al usuario por stdin (o lo recibe como parámetro).
 * Envía REGISTER|<username> y espera REGISTER_OK o ERROR.
 * Si es ERROR muestra el motivo y retorna CLIENT_ERR.
 * Si es OK guarda username, ip propia y status inicial en state.
 */
ClientResult client_register(ClientState *state, const char *username);

/*
 * client_run()
 * Lanza el thread receptor y entra al input loop.
 * Bloquea hasta que el usuario escribe /quit o el servidor manda BYE.
 * Antes de retornar espera a que el thread receptor termine.
 */
ClientResult client_run(ClientState *state);

/*
 * client_disconnect()
 * Envía QUIT, espera BYE, cierra el socket y libera recursos.
 * Seguro de llamar aunque el servidor ya haya cerrado la conexión.
 */
void client_disconnect(ClientState *state);

/* ═══════════════════════════════════════════════════════════════
 * 5. input_loop.c
 * ═══════════════════════════════════════════════════════════════ */

/*
 * input_loop()
 * Loop principal del hilo de entrada (corre en el hilo main).
 * Lee líneas de stdin, las parsea con parse_command() y:
 *   - Si es CMD_TYPE_HELP → llama cmd_help()  (local)
 *   - Si es CMD_TYPE_QUIT → envía QUIT y retorna CLIENT_QUIT
 *   - Resto             → convierte a mensaje de protocolo y envía
 * Retorna cuando el usuario escribe /quit o state->running == 0.
 */
ClientResult input_loop(ClientState *state);

/*
 * parse_command()
 * Parsea una línea de texto crudo de stdin a ParsedCommand.
 * Siempre rellena out (con CMD_TYPE_UNKNOWN si no reconoce).
 *
 * Ejemplos:
 *   "/list"              → { CMD_TYPE_LIST,   "",     ""         }
 *   "/info cami"         → { CMD_TYPE_INFO,   "cami", ""         }
 *   "/status ocupado"    → { CMD_TYPE_STATUS, "OCUPADO", ""      }
 *   "/msg nery hola!"    → { CMD_TYPE_MSG,    "nery", "hola!"    }
 *   "/all buenas noches" → { CMD_TYPE_ALL,    "",     "buenas noches" }
 *   "/quit"              → { CMD_TYPE_QUIT,   "",     ""         }
 *   "/help"              → { CMD_TYPE_HELP,   "",     ""         }
 */
void parse_command(const char *line, ParsedCommand *out);

/*
 * command_to_proto()
 * Convierte un ParsedCommand ya validado a un string de protocolo listo para enviar.
 * Escribe el resultado en buf (tamaño MAX_MSG_LEN).
 * Retorna CLIENT_OK si la conversión es válida, CLIENT_ERR si el comando
 * no debería viajar al servidor (ej: HELP) o si faltan campos requeridos.
 */
ClientResult command_to_proto(const ParsedCommand *cmd, char *buf);

/* ═══════════════════════════════════════════════════════════════
 * 6. receiver_loop.c
 * ═══════════════════════════════════════════════════════════════ */

/*
 * receiver_loop()
 * Función del thread receptor. Firma compatible con pthread_create().
 * Arg: puntero a ClientState.
 *
 * Escucha del socket en loop:
 *   - DELIVER          → ui_print_private_in()
 *   - BROADCAST_FROM   → ui_print_broadcast_in()
 *   - STATUS_CHANGED   → ui_print_status_changed()
 *   - LIST_OK          → ui_print_list()
 *   - INFO_OK          → ui_print_info()
 *   - STATUS_OK        → ui_print_status_ok()
 *   - MSG_OK           → ui_print_msg_ok()
 *   - BROADCAST_OK     → ui_print_broadcast_ok()
 *   - REGISTER_OK      → (solo durante registro, ignorado aquí)
 *   - ERROR            → ui_print_error()
 *   - BYE              → setea state->running = 0 y termina
 *   - recv == 0        → servidor cerró conexión, termina
 */
void *receiver_loop(void *arg);

/*
 * recv_line()
 * Lee del socket byte a byte hasta encontrar '\n' o llenar buf.
 * Retorna número de bytes leídos, 0 si el servidor cerró, -1 si error.
 * Garantiza que buf termina en '\0'.
 */
int recv_line(int sockfd, char *buf, int max_len);

/* ═══════════════════════════════════════════════════════════════
 * 7. commands_local.c
 * ═══════════════════════════════════════════════════════════════ */

/*
 * cmd_help()
 * Imprime en consola la lista de comandos disponibles.
 * No envía nada al servidor.
 */
void cmd_help(void);

/* ═══════════════════════════════════════════════════════════════
 * 8. ui.c  — funciones de formato de consola
 *
 * Convención de prefijos en la salida:
 *   >            línea de prompt del usuario
 *   [ok]         confirmación de operación exitosa
 *   [error]      mensaje de error del servidor
 *   <privado>    mensaje directo recibido
 *   <broadcast>  mensaje general recibido
 *   [info]       información de usuario / lista
 *   [status]     cambio de status (propio o ajeno)
 *   [sistema]    eventos del sistema (conexión, desconexión)
 * ═══════════════════════════════════════════════════════════════ */

/* Imprime el banner inicial al conectarse y registrarse */
void ui_print_welcome(const ClientState *state);

/* Imprime el prompt "> " sin newline (para que el usuario escriba en la misma línea) */
void ui_print_prompt(void);

/* Mensajes entrantes */
void ui_print_private_in(const ClientState *state,
                          const char *from, const char *msg);    /* DELIVER            */
void ui_print_broadcast_in(const ClientState *state,
                            const char *from, const char *msg);  /* BROADCAST_FROM     */
void ui_print_status_changed(const ClientState *state,
                              const char *user,
                              const char *new_status);           /* STATUS_CHANGED     */

/* Confirmaciones de operaciones del usuario */
void ui_print_msg_ok(const char *dest);                          /* MSG_OK             */
void ui_print_broadcast_ok(void);                                /* BROADCAST_OK       */
void ui_print_status_ok(ClientState *state, const char *status); /* STATUS_OK          */

/* Respuestas a consultas */
void ui_print_list(const char *users_raw);   /* LIST_OK  — users_raw = "cami;nery;..."  */
void ui_print_info(const char *username,
                   const char *ip,
                   const char *status);      /* INFO_OK                                 */

/* Errores y sistema */
void ui_print_error(const char *operation,
                    const char *code,
                    const char *description); /* ERROR                                  */
void ui_print_disconnected(void);             /* cuando el servidor manda BYE o cierra  */

#endif /* CLIENT_H */