#ifndef SERVER_h
#define SERVER_H

#define SERVER_PORT 1313

int setup_server(server_ctx *server);
void handle_child(int server_fd, server_ctx *server);

#endif