#ifndef SERVER_H
#define SERVER_H

#include <signal.h>
#include <stddef.h>
#include <sys/types.h>

#include "common.h"
#include "session.h"
#include "parser.h"

extern int g_server_sockfd;
extern client_session_t g_sessions[MAX_CLIENTS];
extern pthread_mutex_t g_sessions_mutex;
extern volatile sig_atomic_t g_server_running;
extern int g_inactivity_timeout_seconds;

int server_init(int port);
int server_start_accept_loop(void);
void server_shutdown(void);

void sessions_init(void);
int sessions_add(client_session_t *session);
void sessions_remove_by_sockfd(int sockfd);
client_session_t *sessions_find_by_sockfd(int sockfd);
client_session_t *sessions_find_by_username(const char *username);
client_session_t *sessions_find_by_ip(const char *ip);
void sessions_list_usernames(char *out, size_t out_size);

int session_username_exists(const char *username);
int session_ip_exists(const char *ip);
int session_mark_inactive_if_needed(client_session_t *session, time_t now,
                                    int timeout_seconds);

int handle_parsed_message(client_session_t *session,
                          const parsed_message_t *msg);
void *client_thread_main(void *arg);
void *inactivity_monitor_main(void *arg);

int send_line(int sockfd, const char *line);
ssize_t recv_line(int sockfd, char *buffer, size_t size);

#endif
