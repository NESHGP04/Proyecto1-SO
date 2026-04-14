<<<<<<< HEAD
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
 
 #define UNUSED(x) ((void)(x))
 
 int sockaddr_to_ip_string(const struct sockaddr_in *addr, char *out, size_t size);
 
 #endif
=======
 // include/common.h 
 #ifndef COMMON_H
 #define COMMON_H
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 #include <unistd.h>
 
 #define MAX_MSG 1024
 #define MAX_USER 32
 
 #endif
>>>>>>> main
