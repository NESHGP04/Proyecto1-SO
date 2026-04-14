/*
 * protocol.h
 * ─────────────────────────────────────────────────────────────────
 * Contrato de protocolo compartido entre cliente y servidor.
 * CC3064 Sistemas Operativos — Proyecto 01: Chat Multithread
 *
 * Grupo:
 *   Camila Richter  - 23183  (protocolo, parsing y handlers del servidor)
 *   Marinés García  - 23391  (cliente por consola)
 *   Nery Molina     - 23218  (núcleo del servidor)
 *
 * REGLA: nadie modifica este archivo sin avisar al grupo.
 *        Cualquier cambio rompe la compatibilidad cliente ↔ servidor.
 * ─────────────────────────────────────────────────────────────────
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

/* ═══════════════════════════════════════════════════════════════
 * 1. LÍMITES Y CONSTANTES DE RED
 * ═══════════════════════════════════════════════════════════════ */

#define PROTO_VERSION       "CHAT/1.0"

#define MAX_MSG_LEN         1024    /* Tamaño máximo de cualquier mensaje del protocolo (bytes) */
#define MAX_USERNAME_LEN    32      /* Longitud máxima de un nombre de usuario                  */
#define MAX_CHAT_LEN        900     /* Longitud máxima del contenido de un mensaje de chat      */
#define MAX_CLIENTS         64      /* Número máximo de clientes simultáneos en el servidor     */

#define INACTIVITY_TIMEOUT  90      /* Segundos sin actividad para marcar usuario como INACTIVO */

#define FIELD_SEP           "|"     /* Separador de campos en el protocolo                      */
#define FIELD_SEP_CHAR      '|'
#define MSG_TERMINATOR      "\n"    /* Terminador de mensaje                                    */
#define MSG_TERMINATOR_CHAR '\n'

/* ═══════════════════════════════════════════════════════════════
 * 2. MENSAJES CLIENTE → SERVIDOR
 *    Formato general: TIPO|campo1|campo2\n
 * ═══════════════════════════════════════════════════════════════ */

/* Registro de usuario al conectarse
 * Formato:  REGISTER|<username>\n
 * Ejemplo:  REGISTER|marines\n               */
#define CMD_REGISTER        "REGISTER"

/* Solicitar lista de usuarios conectados
 * Formato:  LIST\n                           */
#define CMD_LIST            "LIST"

/* Solicitar información de un usuario
 * Formato:  INFO|<username>\n
 * Ejemplo:  INFO|cami\n                      */
#define CMD_INFO            "INFO"

/* Solicitar cambio de propio status
 * Formato:  STATUS|<status>\n
 * Ejemplo:  STATUS|OCUPADO\n                 */
#define CMD_STATUS          "STATUS"

/* Enviar mensaje directo a un usuario
 * Formato:  MSG|<destino>|<mensaje>\n
 * Ejemplo:  MSG|nery|hola!\n                 */
#define CMD_MSG             "MSG"

/* Enviar mensaje a todos los conectados
 * Formato:  BROADCAST|<mensaje>\n
 * Ejemplo:  BROADCAST|buenas noches\n        */
#define CMD_BROADCAST       "BROADCAST"

/* Desconectarse limpiamente
 * Formato:  QUIT\n                           */
#define CMD_QUIT            "QUIT"

/* ═══════════════════════════════════════════════════════════════
 * 3. MENSAJES SERVIDOR → CLIENTE
 * ═══════════════════════════════════════════════════════════════ */

/* Bienvenida al conectarse (antes del REGISTER)
 * Formato:  WELCOME|CHAT/1.0\n               */
#define SRV_WELCOME         "WELCOME"

/* Registro aceptado
 * Formato:  REGISTER_OK|<username>|<ip>|ACTIVO\n
 * Ejemplo:  REGISTER_OK|marines|192.168.1.11|ACTIVO\n */
#define SRV_REGISTER_OK     "REGISTER_OK"

/* Respuesta al comando LIST
 * Formato:  LIST_OK|<user1>;<user2>;<user3>\n
 * Ejemplo:  LIST_OK|cami;marines;nery\n       */
#define SRV_LIST_OK         "LIST_OK"

/* Respuesta al comando INFO
 * Formato:  INFO_OK|<username>|<ip>|<status>\n
 * Ejemplo:  INFO_OK|nery|192.168.1.12|ACTIVO\n */
#define SRV_INFO_OK         "INFO_OK"

/* Confirmación de cambio de status
 * Formato:  STATUS_OK|<status>\n
 * Ejemplo:  STATUS_OK|OCUPADO\n              */
#define SRV_STATUS_OK       "STATUS_OK"

/* Confirmación de envío de mensaje directo
 * Formato:  MSG_OK|<destino>\n
 * Ejemplo:  MSG_OK|cami\n                   */
#define SRV_MSG_OK          "MSG_OK"

/* Entrega de mensaje directo al destinatario
 * Formato:  DELIVER|<origen>|<mensaje>\n
 * Ejemplo:  DELIVER|cami|hola de regreso\n   */
#define SRV_DELIVER         "DELIVER"

/* Confirmación de broadcast enviado
 * Formato:  BROADCAST_OK\n                  */
#define SRV_BROADCAST_OK    "BROADCAST_OK"

/* Entrega de mensaje broadcast a cada cliente
 * Formato:  BROADCAST_FROM|<origen>|<mensaje>\n
 * Ejemplo:  BROADCAST_FROM|nery|buenas noches\n */
#define SRV_BROADCAST_FROM  "BROADCAST_FROM"

/* Notificación de cambio de status de otro usuario
 * (también usado cuando el servidor asigna INACTIVO)
 * Formato:  STATUS_CHANGED|<username>|<status>\n
 * Ejemplo:  STATUS_CHANGED|marines|INACTIVO\n */
#define SRV_STATUS_CHANGED  "STATUS_CHANGED"

/* Respuesta de error para cualquier operación
 * Formato:  ERROR|<operacion>|<codigo>|<descripcion>\n
 * Ejemplo:  ERROR|REGISTER|DUPLICATE_NAME|El nombre ya está en uso\n */
#define SRV_ERROR           "ERROR"

/* Confirmación de desconexión (respuesta a QUIT)
 * Formato:  BYE\n                           */
#define SRV_BYE             "BYE"

/* ═══════════════════════════════════════════════════════════════
 * 4. VALORES DE STATUS
 * ═══════════════════════════════════════════════════════════════ */

#define STATUS_ACTIVE       "ACTIVO"
#define STATUS_BUSY         "OCUPADO"
#define STATUS_INACTIVE     "INACTIVO"

/* ═══════════════════════════════════════════════════════════════
 * 5. CÓDIGOS DE ERROR
 * ═══════════════════════════════════════════════════════════════ */

#define ERR_DUPLICATE_NAME  "DUPLICATE_NAME"   /* Username ya registrado              */
#define ERR_DUPLICATE_IP    "DUPLICATE_IP"     /* IP ya tiene sesión activa           */
#define ERR_USER_NOT_FOUND  "USER_NOT_FOUND"   /* Usuario no existe o no está online  */
#define ERR_INVALID_STATUS  "INVALID_STATUS"   /* Status no reconocido                */
#define ERR_UNKNOWN_CMD     "UNKNOWN_CMD"      /* Tipo de mensaje no reconocido       */
#define ERR_BAD_FORMAT      "BAD_FORMAT"       /* Mensaje malformado (faltan campos)  */

/* ═══════════════════════════════════════════════════════════════
 * 6. MACRO DE CONSTRUCCIÓN DE MENSAJES
 *    Uso: char buf[MAX_MSG_LEN];
 *         PROTO_BUILD(buf, CMD_REGISTER, "marines");
 *         → buf == "REGISTER|marines\n"
 * ═══════════════════════════════════════════════════════════════ */

#include <stdio.h>
#include <string.h>

/*
 * proto_build() — construye un mensaje de protocolo en `buf`.
 * Acepta hasta 3 campos opcionales (pasar NULL si no aplica).
 * Siempre agrega el terminador \n al final.
 * Retorna la longitud del mensaje construido.
 */
static inline int proto_build(char *buf, size_t buf_size,
                               const char *type,
                               const char *f1,
                               const char *f2,
                               const char *f3)
{
    int n = 0;
    n += snprintf(buf + n, buf_size - n, "%s", type);
    if (f1 && n < (int)buf_size) n += snprintf(buf + n, buf_size - n, "|%s", f1);
    if (f2 && n < (int)buf_size) n += snprintf(buf + n, buf_size - n, "|%s", f2);
    if (f3 && n < (int)buf_size) n += snprintf(buf + n, buf_size - n, "|%s", f3);
    if (n < (int)buf_size)        n += snprintf(buf + n, buf_size - n, "\n");
    return n;
}

/* Helpers de construcción de mensajes del cliente */
#define PROTO_REGISTER(buf, user)           proto_build(buf, MAX_MSG_LEN, CMD_REGISTER,   user,  NULL,  NULL)
#define PROTO_LIST(buf)                     proto_build(buf, MAX_MSG_LEN, CMD_LIST,        NULL,  NULL,  NULL)
#define PROTO_INFO(buf, user)               proto_build(buf, MAX_MSG_LEN, CMD_INFO,        user,  NULL,  NULL)
#define PROTO_STATUS(buf, status)           proto_build(buf, MAX_MSG_LEN, CMD_STATUS,      status,NULL,  NULL)
#define PROTO_MSG(buf, dest, msg)           proto_build(buf, MAX_MSG_LEN, CMD_MSG,         dest,  msg,   NULL)
#define PROTO_BROADCAST(buf, msg)           proto_build(buf, MAX_MSG_LEN, CMD_BROADCAST,   msg,   NULL,  NULL)
#define PROTO_QUIT(buf)                     proto_build(buf, MAX_MSG_LEN, CMD_QUIT,        NULL,  NULL,  NULL)

/*
 * proto_parse_type() — extrae el tipo de mensaje de una línea recibida.
 * Escribe el tipo en `type_out` (buffer de al menos MAX_MSG_LEN bytes).
 * Retorna puntero al primer campo después del tipo, o NULL si está vacío.
 *
 * Ejemplo:
 *   char line[] = "DELIVER|cami|hola\n";
 *   char type[MAX_MSG_LEN];
 *   char *rest = proto_parse_type(line, type);
 *   → type == "DELIVER", rest apunta a "cami|hola"
 */
static inline char *proto_parse_type(char *line, char *type_out)
{
    /* Remover \n al final */
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

    char *sep = strchr(line, FIELD_SEP_CHAR);
    if (!sep) {
        /* Mensaje sin campos adicionales (ej: "LIST", "BROADCAST_OK") */
        strncpy(type_out, line, MAX_MSG_LEN - 1);
        type_out[MAX_MSG_LEN - 1] = '\0';
        return NULL;
    }
    size_t type_len = (size_t)(sep - line);
    strncpy(type_out, line, type_len);
    type_out[type_len] = '\0';
    return sep + 1; /* primer campo después del tipo */
}

#endif /* PROTOCOL_H */
