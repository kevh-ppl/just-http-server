#ifndef SERVER_h
#define SERVER_H

#define BUFFER_LENGTH 8192
#define SERVER_PORT 1313

#define HTTP_VERSION "HTTP/1.1"
#define BASE_PATH_WWW "./www"
#define LEN_BASE_PATH_WWW strlen(BASE_PATH_WWW)
#define INDEX_FILE "index.html"
#define LEN_INDEX_FILE strlen(INDEX_FILE)

int setup_server(server_ctx *server);
void handle_child(int server_fd, server_ctx *server);

typedef struct request_ctx
{
    char *response;
    char *resource;
    char *http_version;
    char *lines[];
} request_ctx;
typedef int (*handler_method_fn)(request_ctx *req_ctx);

#endif