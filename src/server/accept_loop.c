#include "../../include/server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int g_server_sockfd = -1;
volatile sig_atomic_t g_server_running = 0;
int g_inactivity_timeout_seconds = DEFAULT_INACTIVITY_SECONDS;

int server_init(int port)
{
    int optval;
    struct sockaddr_in server_addr;

    g_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_server_sockfd < 0) {
        perror("[SERVER] socket");
        return -1;
    }

    optval = 1;
    if (setsockopt(g_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval)) < 0) {
        perror("[SERVER] setsockopt");
        close(g_server_sockfd);
        g_server_sockfd = -1;
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(g_server_sockfd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("[SERVER] bind");
        close(g_server_sockfd);
        g_server_sockfd = -1;
        return -1;
    }

    if (listen(g_server_sockfd, BACKLOG) < 0) {
        perror("[SERVER] listen");
        close(g_server_sockfd);
        g_server_sockfd = -1;
        return -1;
    }

    g_server_running = 1;
    printf("[SERVER] listening on port %d\n", port);
    return 0;
}

int server_start_accept_loop(void)
{
    while (g_server_running) {
        client_session_t session;
        client_session_t *session_slot;
        socklen_t client_len;
        int client_sockfd;
        pthread_t thread_id;
        char ip[INET_ADDRSTRLEN];

        memset(&session, 0, sizeof(session));
        client_len = (socklen_t)sizeof(session.addr);
        client_sockfd = accept(g_server_sockfd, (struct sockaddr *)&session.addr,
                               &client_len);
        if (client_sockfd < 0) {
            if (!g_server_running) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            perror("[SERVER] accept");
            continue;
        }

        if (sockaddr_to_ip_string(&session.addr, ip, sizeof(ip)) != 0) {
            strncpy(ip, "unknown", sizeof(ip) - 1U);
            ip[sizeof(ip) - 1U] = '\0';
        }

        session.sockfd = client_sockfd;
        session.status = ST_ACTIVO;
        session.last_activity = time(NULL);
        session.is_registered = 0;
        session.is_active = 1;

        if (sessions_add(&session) != 0) {
            printf("[SERVER] rejecting connection from %s:%u (server full)\n",
                   ip, (unsigned)ntohs(session.addr.sin_port));
            close(client_sockfd);
            continue;
        }

        session_slot = sessions_find_by_sockfd(client_sockfd);
        if (session_slot == NULL) {
            close(client_sockfd);
            continue;
        }

        printf("[SERVER] new connection from %s:%u\n",
               ip, (unsigned)ntohs(session.addr.sin_port));

        if (pthread_create(&thread_id, NULL, client_thread_main, session_slot) != 0) {
            perror("[SERVER] pthread_create");
            sessions_remove_by_sockfd(client_sockfd);
            continue;
        }

        pthread_mutex_lock(&g_sessions_mutex);
        session_slot->thread_id = thread_id;
        pthread_mutex_unlock(&g_sessions_mutex);
    }

    return 0;
}

void server_shutdown(void)
{
    size_t index;

    if (!g_server_running && g_server_sockfd < 0) {
        return;
    }

    g_server_running = 0;

    if (g_server_sockfd >= 0) {
        shutdown(g_server_sockfd, SHUT_RDWR);
        close(g_server_sockfd);
        g_server_sockfd = -1;
    }

    pthread_mutex_lock(&g_sessions_mutex);
    for (index = 0U; index < MAX_CLIENTS; ++index) {
        if (g_sessions[index].is_active && g_sessions[index].sockfd >= 0) {
            shutdown(g_sessions[index].sockfd, SHUT_RDWR);
        }
    }
    pthread_mutex_unlock(&g_sessions_mutex);
}
