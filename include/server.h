#ifndef SERVER_h
#define SERVER_H

#define BUFFER_LENGTH 4096
#define SERVER_PORT 1313

#define MAX_TOKEN_LINES_REQUEST 50

#define HTTP_VERSION "HTTP/1.1"
#define BASE_PATH_WWW "./www"
#define LEN_BASE_PATH_WWW strlen(BASE_PATH_WWW)
#define INDEX_FILE "index.html"
#define LEN_INDEX_FILE strlen(INDEX_FILE)

int setup_server(server_ctx *server);
void handle_child(int server_fd, server_ctx *server);

#endif