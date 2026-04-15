#include "../../include/server.h"
#include "../../include/parser.h" 

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void update_session_activity(int sockfd)
{
    client_session_t *session = sessions_find_by_sockfd(sockfd);
    if (session == NULL) {
        return;
    }

    pthread_mutex_lock(&g_sessions_mutex);
    if (session->is_active) {
        session->last_activity = time(NULL);
        if (session->status == ST_INACTIVO) {
            session->status = ST_ACTIVO;
        }
    }
    pthread_mutex_unlock(&g_sessions_mutex);
}

static int register_session_if_needed(client_session_t *session)
{
    if (session == NULL) {
        return -1;
    }

    /* Hook para integrar REGISTER real sin tocar infraestructura. */
    return 0;
}

void *client_thread_main(void *arg)
{
    char buffer[MAX_LINE_LEN];
    client_session_t *session;
    int sockfd;
    char ip[INET_ADDRSTRLEN];

    session = (client_session_t *)arg;
    if (session == NULL) {
        return NULL;
    }

    if (pthread_detach(pthread_self()) != 0) {
        perror("[SERVER] pthread_detach");
    }

    sockfd = session->sockfd;
    if (sockaddr_to_ip_string(&session->addr, ip, sizeof(ip)) != 0) {
        strncpy(ip, "unknown", sizeof(ip) - 1U);
        ip[sizeof(ip) - 1U] = '\0';
    }

    if (register_session_if_needed(session) != 0) {
        sessions_remove_by_sockfd(sockfd);
        return NULL;
    }

    if (send_line(sockfd, "WELCOME|CHAT/1.0\n") != 0) {
        perror("[SERVER] send welcome");
        sessions_remove_by_sockfd(sockfd);
        return NULL;
    }

    while (g_server_running) {
        parsed_message_t parsed;
        ssize_t read_bytes = recv_line(sockfd, buffer, sizeof(buffer));

        if (read_bytes > 0) {
            update_session_activity(sockfd);

            if (strcmp(buffer, "PING\n") == 0) {
                if (send_line(sockfd, "PONG\n") != 0) {
                    perror("[SERVER] send PONG");
                    break;
                }
                continue;
            }

            if (strcmp(buffer, "QUIT\n") == 0) {
                send_line(sockfd, "BYE\n");
                printf("[SERVER] client disconnected cleanly from %s:%u\n",
                       ip, (unsigned)ntohs(session->addr.sin_port));
                break;
            }

            if (protocol_parse(buffer, &parsed) == 0) {
                if (handle_parsed_message(session, &parsed) < 0) {
                    if (send_line(sockfd,
                                  "ERROR|PROTO|NOT_IMPLEMENTED|Parser pending\n") != 0) {
                        perror("[SERVER] send protocol stub");
                        break;
                    }
                }
            } else {
                if (send_line(sockfd,
                              "ERROR|PROTO|NOT_IMPLEMENTED|Parser pending\n") != 0) {
                    perror("[SERVER] send protocol pending");
                    break;
                }
            }
            continue;
        }

        if (read_bytes == 0) {
            printf("[SERVER] client disconnected abruptly from %s:%u\n",
                   ip, (unsigned)ntohs(session->addr.sin_port));
        } else {
            printf("[SERVER] recv error from %s:%u: %s\n",
                   ip, (unsigned)ntohs(session->addr.sin_port), strerror(errno));
        }
        break;
    }

    sessions_remove_by_sockfd(sockfd);
    return NULL;
}
