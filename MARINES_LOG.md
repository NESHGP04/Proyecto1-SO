 implementaste, qué decisiones tomaste y qué falta

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