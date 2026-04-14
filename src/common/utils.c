#include "../../include/common.h"

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void safe_copy(char *dst, const char *src, size_t n)
{
    if (dst == NULL || src == NULL || n == 0U) {
        return;
    }

    strncpy(dst, src, n - 1U);
    dst[n - 1U] = '\0';
}

int send_line(int sockfd, const char *line)
{
    size_t total_sent;
    size_t line_len;

    if (line == NULL) {
        errno = EINVAL;
        return -1;
    }

    total_sent = 0U;
    line_len = strlen(line);
    while (total_sent < line_len) {
        ssize_t sent_now = send(sockfd, line + total_sent, line_len - total_sent, 0);
        if (sent_now < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (sent_now == 0) {
            errno = EPIPE;
            return -1;
        }
        total_sent += (size_t)sent_now;
    }

    return 0;
}

ssize_t recv_line(int sockfd, char *buffer, size_t size)
{
    size_t used;

    if (buffer == NULL || size == 0U) {
        errno = EINVAL;
        return -1;
    }

    used = 0U;
    while (used + 1U < size) {
        char ch;
        ssize_t received = recv(sockfd, &ch, 1U, 0);
        if (received == 0) {
            buffer[0] = '\0';
            return 0;
        }
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            buffer[0] = '\0';
            return -1;
        }

        buffer[used++] = ch;
        if (ch == '\n') {
            break;
        }
    }

    buffer[used] = '\0';
    return (ssize_t)used;
}

int sockaddr_to_ip_string(const struct sockaddr_in *addr, char *out, size_t size)
{
    if (addr == NULL || out == NULL || size == 0U) {
        errno = EINVAL;
        return -1;
    }

    if (inet_ntop(AF_INET, &(addr->sin_addr), out, (socklen_t)size) == NULL) {
        return -1;
    }

    return 0;
}
