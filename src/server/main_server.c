#include "../../include/server.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void handle_termination_signal(int signum)
{
    UNUSED(signum);

    g_server_running = 0;
    if (g_server_sockfd >= 0) {
        close(g_server_sockfd);
        g_server_sockfd = -1;
    }
}

static int parse_positive_int(const char *text, int *value)
{
    char *endptr;
    long parsed;

    if (text == NULL || value == NULL) {
        return -1;
    }

    errno = 0;
    parsed = strtol(text, &endptr, 10);
    if (errno != 0 || endptr == text || *endptr != '\0' || parsed <= 0L ||
        parsed > 65535L) {
        return -1;
    }

    *value = (int)parsed;
    return 0;
}

int main(int argc, char *argv[])
{
    int port;
    pthread_t inactivity_thread;
    struct sigaction sa;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Uso: %s <port> [timeout_segundos]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (parse_positive_int(argv[1], &port) != 0) {
        fprintf(stderr, "Puerto invalido: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    if (argc == 3) {
        if (parse_positive_int(argv[2], &g_inactivity_timeout_seconds) != 0) {
            fprintf(stderr, "Timeout invalido: %s\n", argv[2]);
            return EXIT_FAILURE;
        }
    } else {
        g_inactivity_timeout_seconds = DEFAULT_INACTIVITY_SECONDS;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_termination_signal;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) != 0 || sigaction(SIGTERM, &sa, NULL) != 0) {
        perror("[SERVER] sigaction");
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    sessions_init();
    if (server_init(port) != 0) {
        return EXIT_FAILURE;
    }

    if (pthread_create(&inactivity_thread, NULL, inactivity_monitor_main, NULL) != 0) {
        perror("[SERVER] pthread_create inactivity");
        server_shutdown();
        return EXIT_FAILURE;
    }

    server_start_accept_loop();
    printf("[SERVER] shutting down\n");
    server_shutdown();

    if (pthread_join(inactivity_thread, NULL) != 0) {
        perror("[SERVER] pthread_join inactivity");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
