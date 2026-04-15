/*
 * include/common.h
 * ─────────────────────────────────────────────────────────────────
 * Constantes y utilidades base compartidas por cliente y servidor.
 * No incluye ningún header del proyecto — cero dependencias.
 * ─────────────────────────────────────────────────────────────────
 */

#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

/* ── Límites del sistema ─────────────────────────────────────── */
#define MAX_CLIENTS               64
#define MAX_USERNAME_LEN          32
#define MAX_LINE_LEN            1024
#define DEFAULT_INACTIVITY_SECONDS 90
#define BACKLOG                   16

/*
 * Alias de compatibilidad:
 *   MAX_MSG      — usado en parser.h (código del servidor)
 *   MAX_USER     — alias legado
 *   MAX_MSG_LEN  — usado en client.h y protocol.h (código del cliente)
 */
#define MAX_MSG      MAX_LINE_LEN
#define MAX_USER     MAX_USERNAME_LEN
#define MAX_MSG_LEN  MAX_LINE_LEN

/* ── Macro utilitaria ────────────────────────────────────────── */
#define UNUSED(x) ((void)(x))

/* ── Funciones de utilidad ───────────────────────────────────── */

/*
 * safe_copy()
 * strncpy con garantía de terminador nulo.
 */
void safe_copy(char *dst, const char *src, size_t n);

/*
 * sockaddr_to_ip_string()
 * Convierte sockaddr_in a string "192.168.1.10".
 * Retorna 0 si OK, -1 si falla.
 */
int sockaddr_to_ip_string(const struct sockaddr_in *addr,
                           char *out, size_t size);

#endif /* COMMON_H */