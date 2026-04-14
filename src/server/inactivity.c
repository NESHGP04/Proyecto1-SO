#include "../../include/server.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *inactivity_monitor_main(void *arg)
{
    char notifications[MAX_CLIENTS][MAX_LINE_LEN];
    int sockets[MAX_CLIENTS];
    size_t notification_count;

    UNUSED(arg);

    while (g_server_running) {
        size_t index;
        time_t now = time(NULL);

        notification_count = 0U;

        pthread_mutex_lock(&g_sessions_mutex);
        for (index = 0U; index < MAX_CLIENTS; ++index) {
            if (g_sessions[index].is_active &&
                session_mark_inactive_if_needed(&g_sessions[index], now,
                                                g_inactivity_timeout_seconds)) {
                const char *name;

                name = g_sessions[index].username[0] != '\0' ?
                    g_sessions[index].username : "UNREGISTERED";

                printf("[SERVER] user %s marked INACTIVO due to timeout\n", name);

                sockets[notification_count] = g_sessions[index].sockfd;
                snprintf(notifications[notification_count],
                         sizeof(notifications[notification_count]),
                         "STATUS_CHANGED|%s|INACTIVO\n", name);
                ++notification_count;
            }
        }
        pthread_mutex_unlock(&g_sessions_mutex);

        for (index = 0U; index < notification_count; ++index) {
            if (sockets[index] >= 0) {
                if (send_line(sockets[index], notifications[index]) != 0) {
                    perror("[SERVER] send inactivity notification");
                }
            }
        }

        sleep(1);
    }

    return NULL;
}
