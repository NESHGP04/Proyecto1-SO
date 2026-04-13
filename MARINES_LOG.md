Lo que implementé, qué decisiones tomé y qué falta

# 10/04/2026

### 1. protocol.h
Creación base de este archivo, CAMI debe modificarlo después

Sección 1 — Límites (MAX_MSG_LEN, MAX_USERNAME_LEN, etc.) — los usas para declarar tus buffers en el cliente. Nunca hardcodees 1024 en tu código, siempre MAX_MSG_LEN.
Sección 2 — CMD_* — los comandos que tu cliente envía. Los usas para construir mensajes y también para no tipar strings sueltos.
Sección 3 — SRV_* — las respuestas que tu thread receptor va a recibir y comparar con strcmp.
Sección 4-5 — Status y errores — constantes que usas en la UI para mostrar mensajes formateados.
Sección 6 — proto_build() y macros — la parte más útil para vos. En vez de hacer sprintf(buf, "MSG|%s|%s\n", dest, msg) en cada lugar, simplemente llamás PROTO_MSG(buf, dest, msg). Y proto_parse_type() te extrae el tipo de cualquier línea recibida, que es exactamente lo que necesita tu receiver thread.

### 2. client.h
ClientState — la struct central que comparten los dos threads. El campo más crítico es running: el receiver_loop lo pone en 0 cuando llega BYE, y el input_loop lo chequea para saber que debe salir. El print_mutex evita que los dos threads impriman al mismo tiempo y mezclen texto en consola.

ClientResult — en vez de retornar -1 o 0 con significados mágicos, cada función retorna un enum legible. CLIENT_QUIT (usuario escribió /quit) y CLIENT_DISCONN (servidor mandó BYE) son casos distintos aunque ambos terminan el programa.

ParsedCommand — separa el parsing de la lógica de envío. parse_command() convierte texto crudo de stdin al struct, y command_to_proto() convierte el struct a string de protocolo. Así podés testear el parsing por separado sin necesidad de red.

recv_line() — función clave. Lee byte a byte hasta \n. Sin esto, el receiver podría leer medio mensaje o dos mensajes juntos en el mismo buffer.

ui_* — todas las funciones de impresión están aisladas en ui.c. Si quieren cambiar colores o formato para la demo, tocás un solo archivo.

### 3. stub_server.c
WELCOME|CHAT/1.0          ← lo primero que manda siempre
REGISTER_OK|marines|127.0.0.1|ACTIVO   ← registro exitoso
STATUS_OK|OCUPADO         ← cambio de status
BYE                       ← QUIT limpio
El test del duplicado no disparó el error porque el primer cliente ya había cerrado la conexión abruptamente (nc salió antes), así que el slot se liberó. Cuando el servidor real de Nery esté, ese caso sí va a funcionar con dos conexiones simultáneas reales.

FALTA:
compilar: 
gcc -std=c11 -Wall -Wextra -Iinclude tests/stub_server.c -lpthread -o stub_server
./stub_server 8080

probar: 
nc 127.0.0.1 8080
- escribe: REGISTER|marines
- escribe: STATUS|OCUPADO
- escribe: QUIT


## 11/04/2026

### 1.Main_client.c
Conectando a 127.0.0.1:8080...
Conexión establecida.
  Usuario  : marines
  IP propia: 127.0.0.1
  Status   : ACTIVO
  El handshake completo funciona: WELCOME → REGISTER → REGISTER_OK → banner.

Test 2 — username inválido: rechazado localmente antes de tocar el socket, con mensaje claro.

Test 3 — puerto 99999: rechazado con validación antes del connect().

Test 4 — servidor inalcanzable: Connection refused con código de salida 1.

- validate_username() corre en el cliente antes de enviar nada — si el nombre es inválido, ni conectamos. validate_port() igual. Esto evita mensajes de error confusos que vendrían del servidor.
- El handshake está en client_register() separado de main() — cuando llegue la integración con Nery, si cambia algo del registro solo tocás esa función.
- MSG_NOSIGNAL en el send() del disconnect evita que el proceso muera con SIGPIPE si el servidor ya cerró la conexión antes de que mandemos el QUIT.

### 2. receiver_loop.c
TEST
WELCOME|CHAT/1.0               ← handshake
REGISTER_OK|marines|...|ACTIVO ← registro
LIST_OK|marines                ← lista
STATUS_OK|OCUPADO              ← cambio de status
ERROR|MSG|USER_NOT_FOUND|...   ← error manejado
BROADCAST_FROM|marines|...     ← broadcast entrante
BROADCAST_OK                   ← confirmación
BYE                            ← salida limpia

- parse_msg_fields() en vez de strtok para DELIVER y BROADCAST_FROM — si alguien manda el mensaje "hola|como|estas", strtok lo rompería en tres tokens. Esta función toma solo el primer | como separador y el resto va completo al campo mensaje.
- print_mutex alrededor de cada ui_* y ui_print_prompt() — sin esto, el receiver puede imprimir un mensaje entrante justo en medio de lo que el usuario está escribiendo, mezclando los caracteres. El mutex garantiza que primero termina de imprimir el mensaje y luego redibuja el prompt >.
- state->running = 0 al final del thread — cuando el receiver termina (por BYE, EOF o error), le avisa al input_loop poniendo el flag en 0. El input_loop lo chequea y también sale. Así los dos threads se coordinan sin señales ni pipes.

### 3.input_loop.c
TEST VÁLIDO:
/list          → LIST\n
/info cami     → INFO|cami\n
/status ocupado → STATUS|OCUPADO\n   ← normaliza minúsculas a mayúsculas
/msg nery hola como estás → MSG|nery|hola como estás\n
/msg cami texto con | pipe → MSG|cami|texto con | pipe\n  ← | en el mensaje no rompe
/all buenas noches → BROADCAST|buenas noches\n
/quit          → QUIT\n
/help          → (local)  ← nunca viaja al servidor

TEST INVÁLIDO:
/info          → UNKNOWN  (falta usuario)
/status        → UNKNOWN  (falta estado)
/status invalido → STATUS pero proto:(local)  (estado no reconocido)
/msg cami      → UNKNOWN  (falta mensaje)
sin slash      → UNKNOWN

- str_toupper_copy() en /status — el usuario puede escribir /status ocupado, /status Ocupado o /status OCUPADO y siempre llega STATUS|OCUPADO\n al servidor. Sin esto tendrían que documentar que es case-sensitive y en la demo alguien lo escribiría mal.
- Mensajes con | en el contenido — /msg cami texto con | pipe genera MSG|cami|texto con | pipe\n. El servidor toma solo el primer | como separador de destino, el resto es mensaje. Esto ya lo maneja el receiver_loop con parse_msg_fields().
- fgets con EOF (Ctrl+D) — si el usuario cierra stdin en vez de escribir /quit, el cliente igual manda QUIT\n al servidor antes de cerrar. Sin esto, el servidor vería una desconexión abrupta.

### 4.ui.c y commands_local.c
- colors_on() con isatty() — los colores ANSI solo se activan si la salida es una terminal real. Si alguien redirige ./client > log.txt, el archivo no tiene \033[32m basura adentro. Útil si el profesor revisa logs.
- Colores por status — ACTIVO → verde, OCUPADO → amarillo, INACTIVO → gris. Consistente en ui_print_status_ok, ui_print_status_changed y ui_print_info. Un vistazo al chat basta para saber quién está disponible.