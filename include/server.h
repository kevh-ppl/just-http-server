#ifndef SERVER_h
#define SERVER_H

#define BUFFER_LENGTH 1024
#define SERVER_PORT 1313

#define MAX_TOKEN_LINES_REQUEST 50

int setup_server(server_ctx *server);
void handle_child(int server_fd, server_ctx *server);

#endif