#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#define MAX_CLIENTS 64
#define MAX_USERNAME_LEN 32
#define MAX_LINE_LEN 1024
#define DEFAULT_INACTIVITY_SECONDS 90
#define BACKLOG 16

/* Alias de compatibilidad con código legado del parser. */
#define MAX_MSG MAX_LINE_LEN
#define MAX_USER MAX_USERNAME_LEN

#define UNUSED(x) ((void)(x))

void safe_copy(char *dst, const char *src, size_t n);
int sockaddr_to_ip_string(const struct sockaddr_in *addr, char *out, size_t size);

#endif
