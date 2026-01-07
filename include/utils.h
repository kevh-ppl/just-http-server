#ifndef UTILS_H
#define UTILS_H

#include <sys/socket.h>

typedef struct server_ctx
{
    struct sockaddr_in *address;
    socklen_t addr_len;
} server_ctx;

void abort_server(const char *entity, const char *format, ...);
void print_and_keep_going(const char *entity, const char *format, ...);
pid_t do_fork();
void kill_child();

#endif