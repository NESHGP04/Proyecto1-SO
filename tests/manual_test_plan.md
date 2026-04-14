# Plan de Pruebas Manual — Chat Multithread
**CC3064 Sistemas Operativos — Proyecto 01**  
**Autora:** Marinés García - 23391  
**Fecha:** abril 2026

---

## Configuración del entorno de prueba

### Opción A — Con stub server (desarrollo, sin servidor real)
```bash
# Terminal 1: levantar stub
gcc -std=c11 -Wall -Iinclude tests/stub_server.c -lpthread -o stub_server
./stub_server 8080

# Terminal 2 y 3: clientes
./client <username> 127.0.0.1 8080
```

### Opción B — Con servidor real (integración, demo final)
```bash
# Máquina servidor (ej: 192.168.1.10)
./server 8080

# Máquinas cliente
./client <username> 192.168.1.10 8080
```

### Nomenclatura de las máquinas en los casos
| Máquina | Rol | Usuario de prueba |
|---|---|---|
| PC-A | Servidor | — |
| PC-B | Cliente 1 | `marines` |
| PC-C | Cliente 2 | `cami` |
| PC-B (segunda sesión) | Cliente 3 | `nery` |

---

## Casos de prueba

---

### Caso 1 — Cliente se registra exitosamente

**Objetivo:** Verificar que el cliente completa el handshake y muestra el banner.

**Precondición:** Servidor corriendo, ningún usuario conectado.

**Pasos:**
1. En PC-B ejecutar: `./client marines 192.168.1.10 8080`

**Resultado esperado:**
```
Conectando a 192.168.1.10:8080...
Conexión establecida.

╔══════════════════════════════════════════╗
║  Chat Multithread  CCc3064 - UVG         ║
╚══════════════════════════════════════════╝
  Servidor : 192.168.1.10:8080
  Usuario  : marines
  IP propia: 192.168.1.11
  Status   : ACTIVO

  Escribe /help para ver los comandos.

────────────────────────────────────────
>
```

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 2 — Segundo cliente se registra

**Objetivo:** Verificar que el servidor soporta múltiples clientes simultáneos.

**Precondición:** `marines` ya conectado (Caso 1 exitoso).

**Pasos:**
1. En PC-C ejecutar: `./client cami 192.168.1.10 8080`

**Resultado esperado:**
- PC-C muestra el banner con `Usuario: cami`
- PC-B **no** es interrumpido ni ve ningún mensaje por esta conexión
- El servidor tiene ahora dos sesiones activas

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 3 — Registro rechazado por username duplicado

**Objetivo:** Verificar que el servidor rechaza usernames ya registrados.

**Precondición:** `marines` ya conectado.

**Pasos:**
1. En PC-C ejecutar: `./client marines 192.168.1.10 8080`

**Resultado esperado en PC-C:**
```
Conectando a 192.168.1.10:8080...
Conexión establecida.
[error] Registro rechazado — DUPLICATE_NAME: El nombre ya está en uso
```
El proceso termina con código de salida distinto de 0.  
PC-B no es afectado.

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 4 — /list devuelve usuarios conectados

**Objetivo:** Verificar listado de usuarios activos.

**Precondición:** `marines` y `cami` conectados.

**Pasos:**
1. En PC-B (marines) escribir: `/list`

**Resultado esperado en PC-B:**
```
[usuarios] 2 conectados
  • marines
  • cami
>
```

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 5 — /info devuelve IP y estado de un usuario

**Objetivo:** Verificar obtención de información de un usuario específico.

**Precondición:** `marines` y `cami` conectados.

**Pasos:**
1. En PC-B (marines) escribir: `/info cami`

**Resultado esperado en PC-B:**
```
[info] cami
  IP     : 192.168.1.12
  Status : ACTIVO
>
```

**Variante — usuario inexistente:**
1. Escribir: `/info fantasma`

**Resultado esperado:**
```
[error] INFO/USER_NOT_FOUND: Usuario no encontrado
>
```

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 6 — /status cambia el estado del usuario

**Objetivo:** Verificar cambio de status propio y notificación a otros.

**Precondición:** `marines` y `cami` conectados.

**Pasos:**
1. En PC-B (marines) escribir: `/status ocupado`

**Resultado esperado en PC-B:**
```
[~] tu status es ahora OCUPADO
>
```

**Resultado esperado en PC-C (cami) — llega automáticamente:**
```
[~] marines ahora está OCUPADO
>
```

**Variante — status inválido:**
1. Escribir: `/status dormido`

**Resultado esperado en PC-B:**
```
[error] Status inválido: 'DORMIDO'
        Opciones: activo | ocupado | inactivo
>
```

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 7 — /msg entrega mensaje privado

**Objetivo:** Verificar mensajería directa entre usuarios.

**Precondición:** `marines` y `cami` conectados.

**Pasos:**
1. En PC-B (marines) escribir: `/msg cami hola, cómo estás?`

**Resultado esperado en PC-B:**
```
[ok] mensaje enviado a cami
>
```

**Resultado esperado en PC-C (cami):**
```
[HH:MM] >> privado  marines → hola, cómo estás?
>
```

**Verificación adicional:** el mensaje **no** debe aparecer en ninguna otra terminal.

**Variante — destinatario no existe:**
1. En PC-B escribir: `/msg fantasma hola`

**Resultado esperado:**
```
[error] MSG/USER_NOT_FOUND: Usuario no encontrado
>
```

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 8 — /all hace broadcast a todos

**Objetivo:** Verificar que el broadcast llega a todos los clientes conectados.

**Precondición:** `marines`, `cami` y `nery` conectados (tres clientes).

**Pasos:**
1. En PC-B (marines) escribir: `/all buenas noches a todos`

**Resultado esperado en PC-B:**
```
[broadcast] enviado
[HH:MM] ## broadcast  marines → buenas noches a todos
>
```

**Resultado esperado en PC-C (cami):**
```
[HH:MM] ## broadcast  marines → buenas noches a todos
>
```

**Resultado esperado en PC-B segunda sesión (nery):**
```
[HH:MM] ## broadcast  marines → buenas noches a todos
>
```

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 9 — Desconexión abrupta de un cliente

**Objetivo:** Verificar que el servidor detecta y limpia sesiones cerradas sin QUIT.

**Precondición:** `marines` y `cami` conectados.

**Pasos:**
1. En PC-B cerrar la ventana del terminal **sin** escribir `/quit`
   (o ejecutar `kill -9 <pid_del_cliente>`)
2. En PC-C (cami) escribir: `/list`

**Resultado esperado en PC-C:**
```
[usuarios] 1 conectados
  • cami
>
```
`marines` ya no aparece en la lista — el servidor limpió la sesión.

**Verificación adicional:**
- Intentar registrar un nuevo cliente con username `marines` → debe ser exitoso
  (la sesión fue liberada correctamente)

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 10 — Cliente queda inactivo y es notificado

**Objetivo:** Verificar que el servidor asigna INACTIVO tras el timeout de inactividad.

**Precondición:** `marines` y `cami` conectados.  
**Nota:** El timeout está configurado en 90 segundos (constante `INACTIVITY_TIMEOUT`
en `protocol.h`). Para la demo reducirlo a 15–20 segundos en el código del servidor.

**Pasos:**
1. Conectar `marines` y no escribir nada durante el período de inactividad
2. Esperar a que el servidor envíe el cambio de status

**Resultado esperado en PC-B (marines) — llega automáticamente:**
```
[~] marines ahora está INACTIVO
>
```

**Resultado esperado en PC-C (cami) — también llega:**
```
[~] marines ahora está INACTIVO
>
```

**Verificación adicional:** `marines` puede seguir enviando mensajes
(el status cambia pero la conexión no se cierra).

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

### Caso 11 — /quit cierra la sesión limpiamente

**Objetivo:** Verificar desconexión controlada con notificación al servidor.

**Precondición:** `marines` y `cami` conectados.

**Pasos:**
1. En PC-B (marines) escribir: `/quit`

**Resultado esperado en PC-B:**
```
[sistema] Desconectado del servidor.

Hasta luego.
```
El proceso termina con código de salida 0.

**Resultado esperado en PC-C (cami):**
- No recibe ningún mensaje de error
- Al escribir `/list` ya no aparece `marines`

**Verificación adicional — Ctrl+D como alternativa:**
1. Conectar un cliente, en vez de `/quit` presionar Ctrl+D
2. Verificar que el servidor también libera la sesión limpiamente

**Resultado obtenido:** _______________

**Estado:** [ ] PASS  [ ] FAIL

---

## Resumen de resultados

| # | Caso | Estado |
|---|------|--------|
| 1 | Registro exitoso | [ ] PASS  [ ] FAIL |
| 2 | Segundo cliente se registra | [ ] PASS  [ ] FAIL |
| 3 | Registro rechazado (duplicado) | [ ] PASS  [ ] FAIL |
| 4 | /list devuelve conectados | [ ] PASS  [ ] FAIL |
| 5 | /info devuelve IP y estado | [ ] PASS  [ ] FAIL |
| 6 | /status cambia estado | [ ] PASS  [ ] FAIL |
| 7 | /msg entrega privado | [ ] PASS  [ ] FAIL |
| 8 | /all hace broadcast | [ ] PASS  [ ] FAIL |
| 9 | Desconexión abrupta | [ ] PASS  [ ] FAIL |
| 10 | Inactividad → INACTIVO | [ ] PASS  [ ] FAIL |
| 11 | /quit cierra limpio | [ ] PASS  [ ] FAIL |

**Total PASS:** _____ / 11

---

## Notas de la prueba

_(Anotar aquí cualquier comportamiento inesperado, mensajes de error del sistema,
diferencias entre stub server y servidor real, etc.)_

_______________________________________________________________________________

_______________________________________________________________________________

_______________________________________________________________________________