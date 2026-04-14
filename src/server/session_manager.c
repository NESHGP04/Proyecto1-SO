#include "../../include/server.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

client_session_t g_sessions[MAX_CLIENTS];
pthread_mutex_t g_sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

static void clear_session_slot(client_session_t *session)
{
    if (session == NULL) {
        return;
    }

    memset(session, 0, sizeof(*session));
    session->sockfd = -1;
    session->status = ST_ACTIVO;
}

const char *status_to_string(user_status_t status)
{
    switch (status) {
        case ST_ACTIVO:
            return "ACTIVO";
        case ST_OCUPADO:
            return "OCUPADO";
        case ST_INACTIVO:
            return "INACTIVO";
        default:
            return "DESCONOCIDO";
    }
}

void sessions_init(void)
{
    size_t index;

    pthread_mutex_lock(&g_sessions_mutex);
    for (index = 0U; index < MAX_CLIENTS; ++index) {
        clear_session_slot(&g_sessions[index]);
    }
    pthread_mutex_unlock(&g_sessions_mutex);
}

int sessions_add(client_session_t *session)
{
    size_t index;
    int result;

    if (session == NULL) {
        return -1;
    }

    result = -1;
    pthread_mutex_lock(&g_sessions_mutex);
    for (index = 0U; index < MAX_CLIENTS; ++index) {
        if (!g_sessions[index].is_active) {
            g_sessions[index] = *session;
            g_sessions[index].is_active = 1;
            result = 0;
            break;
        }
    }
    pthread_mutex_unlock(&g_sessions_mutex);

    return result;
}

void sessions_remove_by_sockfd(int sockfd)
{
    size_t index;

    pthread_mutex_lock(&g_sessions_mutex);
    for (index = 0U; index < MAX_CLIENTS; ++index) {
        if (g_sessions[index].is_active && g_sessions[index].sockfd == sockfd) {
            int old_sockfd = g_sessions[index].sockfd;

            g_sessions[index].is_active = 0;
            g_sessions[index].is_registered = 0;
            g_sessions[index].thread_id = (pthread_t)0;
            g_sessions[index].status = ST_INACTIVO;
            g_sessions[index].sockfd = -1;
            memset(g_sessions[index].username, 0, sizeof(g_sessions[index].username));
            memset(&g_sessions[index].addr, 0, sizeof(g_sessions[index].addr));
            g_sessions[index].last_activity = 0;

            pthread_mutex_unlock(&g_sessions_mutex);
            if (old_sockfd >= 0) {
                shutdown(old_sockfd, SHUT_RDWR);
                close(old_sockfd);
            }
            return;
        }
    }
    pthread_mutex_unlock(&g_sessions_mutex);
}

client_session_t *sessions_find_by_sockfd(int sockfd)
{
    size_t index;
    client_session_t *found;

    found = NULL;
    pthread_mutex_lock(&g_sessions_mutex);
    for (index = 0U; index < MAX_CLIENTS; ++index) {
        if (g_sessions[index].is_active && g_sessions[index].sockfd == sockfd) {
            found = &g_sessions[index];
            break;
        }
    }
    pthread_mutex_unlock(&g_sessions_mutex);

    return found;
}

client_session_t *sessions_find_by_username(const char *username)
{
    size_t index;
    client_session_t *found;

    if (username == NULL || username[0] == '\0') {
        return NULL;
    }

    found = NULL;
    pthread_mutex_lock(&g_sessions_mutex);
    for (index = 0U; index < MAX_CLIENTS; ++index) {
        if (g_sessions[index].is_active &&
            g_sessions[index].is_registered &&
            strcmp(g_sessions[index].username, username) == 0) {
            found = &g_sessions[index];
            break;
        }
    }
    pthread_mutex_unlock(&g_sessions_mutex);

    return found;
}

client_session_t *sessions_find_by_ip(const char *ip)
{
    size_t index;
    client_session_t *found;

    if (ip == NULL || ip[0] == '\0') {
        return NULL;
    }

    found = NULL;
    pthread_mutex_lock(&g_sessions_mutex);
    for (index = 0U; index < MAX_CLIENTS; ++index) {
        if (g_sessions[index].is_active) {
            char current_ip[INET_ADDRSTRLEN];
            if (sockaddr_to_ip_string(&g_sessions[index].addr, current_ip,
                                      sizeof(current_ip)) == 0 &&
                strcmp(current_ip, ip) == 0) {
                found = &g_sessions[index];
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_sessions_mutex);

    return found;
}

void sessions_list_usernames(char *out, size_t out_size)
{
    size_t index;
    size_t used;
    int first;

    if (out == NULL || out_size == 0U) {
        return;
    }

    out[0] = '\0';
    used = 0U;
    first = 1;

    pthread_mutex_lock(&g_sessions_mutex);
    for (index = 0U; index < MAX_CLIENTS; ++index) {
        int written;

        if (!g_sessions[index].is_active ||
            !g_sessions[index].is_registered ||
            g_sessions[index].username[0] == '\0') {
            continue;
        }

        written = snprintf(out + used, out_size - used, "%s%s",
                           first ? "" : ";", g_sessions[index].username);
        if (written < 0 || (size_t)written >= out_size - used) {
            break;
        }
        used += (size_t)written;
        first = 0;
    }
    pthread_mutex_unlock(&g_sessions_mutex);
}

int session_username_exists(const char *username)
{
    return sessions_find_by_username(username) != NULL ? 1 : 0;
}

int session_ip_exists(const char *ip)
{
    return sessions_find_by_ip(ip) != NULL ? 1 : 0;
}

int session_mark_inactive_if_needed(client_session_t *session, time_t now,
                                    int timeout_seconds)
{
    double elapsed;

    if (session == NULL || !session->is_active) {
        return 0;
    }

    if (session->status == ST_INACTIVO) {
        return 0;
    }

    elapsed = difftime(now, session->last_activity);
    if (elapsed < (double)timeout_seconds) {
        return 0;
    }

    session->status = ST_INACTIVO;
    return 1;
}
