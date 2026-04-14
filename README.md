# Proyecto1-SO

Implementación base del núcleo del servidor multithread en C para el chat del proyecto de Sistemas Operativos.

## Qué implementa esta parte

- Socket servidor TCP IPv4 sobre `INADDR_ANY`
- `bind`, `listen` y `accept`
- Un `pthread` detached por cliente
- Arreglo fijo global de sesiones
- Protección con `pthread_mutex_t`
- Detección de desconexión abrupta por `recv() == 0` o error
- Monitor de inactividad que marca sesiones como `INACTIVO`
- Hooks mínimos para integración futura con `protocol_parse(...)` y `handle_parsed_message(...)`

## Compilación

```bash
make
```

## Ejecución

```bash
./server 8080
./server 8080 15
```

## Qué ya funciona

- Arranque del servidor y escucha en el puerto indicado
- Aceptación de múltiples clientes
- Hilo independiente por cliente
- Respuesta temporal a `PING\n` con `PONG\n`
- Respuesta temporal a `QUIT\n` con `BYE\n`
- Limpieza de sesión al desconectarse un cliente
- Cambio automático a `INACTIVO` por timeout

## Pendiente para integración

- Parser real del protocolo
- Registro real de usuarios
- Handlers completos para `REGISTER`, `LIST`, `INFO`, `STATUS`, `MSG`, `BROADCAST` y `QUIT`
- Integración con la lógica de negocio del resto del equipo
