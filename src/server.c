#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>     //stderror(), strlen()
#include <errno.h>      //errno
#include <netinet/in.h> //htons to flip endianess
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/fcntl.h> //open
#include <inttypes.h>  //PRId64

#include "standard.h"
#include "parser.h"
#include "utils.h"
#include "server.h"

// #include <winsock2.h> // for Windows

static int handle_get(request_ctx *req_ctx);
static handler_method_fn handlers[] = {
    [GET] = handle_get,
    [UNKNOWN] = NULL,
};

/*
    @brief Setup and initializes server

    Creates socket AF_INET (IPV_4) and SOCK_STREAM (TCP)
    @return Server FD or -1 on error
*/
int setup_server(server_ctx *server)
{
    // server
    //  first i need a file descriptor for the server
    int server_fd;
    server->address = malloc(sizeof(struct sockaddr_in));
    server->addr_len = sizeof(*server->address);
    int opt = 1;

    /*
    Using SOCK_NONBLOCK flag for socket because I'm already using
    the syscall poll to handle socket events
    */
    /*
    I tried using SOCK_NONBLOCK but it fucked up something
    */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // using SOCL_STREAM sets up our server to go with TCP
    {
        print_and_keep_going("Server", "socket failed");
        return -1;
    }

    // set opt for socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        free(server);
        print_and_keep_going("Server", "setsocketopt");
        return -1;
    }

    server->address->sin_family = AF_INET;          // ipv4
    server->address->sin_addr.s_addr = INADDR_ANY;  // any address
    server->address->sin_port = htons(SERVER_PORT); // we must use htons to convert host endianess to network endianess (big-endian)

    // we had specified the port, now we must bind it
    //  I must to cast it to struct sockaddr*
    if (bind(server_fd, (struct sockaddr *)server->address, server->addr_len) < 0)
    {
        free(server);
        print_and_keep_going("Serverss", "Error binding port: %s", strerror(errno));
        return -1;
    }

    // prepare to accpet conections
    // the second param is the conns that can be queued
    if (listen(server_fd, 3) < 0)
    {
        print_and_keep_going("Server", "shalalala, listening on port %d", SERVER_PORT);
        return -1;
    }

    return server_fd;
}

static int handle_get(request_ctx *req_ctx)
{
    if (req_ctx->resource == NULL || req_ctx->http_version == NULL)
    {
        print_and_keep_going("Server", "Either resource or http version not provided");
        return -1;
    }
    printf("Handling GET request with %s and %s\n", req_ctx->resource, req_ctx->http_version);

    /*
    first i gotta check for the resource
    if it does not exists, return a 404 NOT FOUND response

    i can read first the file, see how many bytes it is,
    then allocate just that and read again

    first / gotta be replaced with BASE_PATH_WWW

    "./web" + resource -> Just gotta do a strcat
    */

    char *path_to_resource;
    if (strcmp(req_ctx->resource, "/") == 0)
    {
        int len_path = LEN_BASE_PATH_WWW + strlen(req_ctx->resource) + LEN_INDEX_FILE + 1;
        path_to_resource = (char *)malloc(len_path);
        if (path_to_resource == NULL)
        {
            print_and_keep_going("Server", "Error allocating memory for path to resource");
            return -1;
        }
        memcpy(path_to_resource, BASE_PATH_WWW, LEN_BASE_PATH_WWW);
        snprintf(path_to_resource, len_path, "%s%s%s", BASE_PATH_WWW, req_ctx->resource, INDEX_FILE);
    }
    else
    {
        int len_path = LEN_BASE_PATH_WWW + strlen(req_ctx->resource) + 1;
        path_to_resource = (char *)malloc(len_path);
        if (path_to_resource == NULL)
        {
            print_and_keep_going("Server", "Error allocating memory for path to resource");
            return -1;
        }
        memcpy(path_to_resource, BASE_PATH_WWW, LEN_BASE_PATH_WWW);
        snprintf(path_to_resource, len_path, "%s%s", BASE_PATH_WWW, req_ctx->resource);
    }

    printf("path_ro_resource: %s\n", path_to_resource);

    struct stat stbuf;
    if (stat(path_to_resource, &stbuf) == -1)
    {
        print_and_keep_going("Server", "Error doing stat for path to resource");
        return -1;
    }

    if (!S_ISREG(stbuf.st_mode))
    {
        print_and_keep_going("Server", "Not a regular file");
        return -1;
    }
    int fd_resource = open(path_to_resource, O_RDONLY);
    if (fd_resource == -1)
    {
        print_and_keep_going("Server", "Error opening resource");
        return -1;
    }

    char *body = (char *)malloc((int64_t)stbuf.st_size);
    printf("st_size: %" PRId64 "\n", stbuf.st_size);
    if (body == NULL)
    {
        print_and_keep_going("Server", "Error allocating memory for body response");
        return -1;
    }

    int nbytes_body = read(fd_resource, body, BUFFER_LENGTH);
    if (nbytes_body == -1)
    {
        print_and_keep_going("Server", "Error reading resource");
        return -1;
    }

    printf("nbytes_read: %d\n", nbytes_body);
    // at this point i can assamble the response with status 200
    // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

    int offset;
    offset = snprintf(req_ctx->response, BUFFER_LENGTH, "%s", HTTP_VERSION);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", SP);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", CODE_OK);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", SP);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", STATUS_OK);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", CRLF);

    // headers
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", KEY_CONTENT_TYPE);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", SP);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", VALUE_CONTENT_TYPE_TEXT);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", "html; charset=utf-8");
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", CRLF);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", KEY_CONTENT_LENGHT);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", SP);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%d", nbytes_body);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", CRLF);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", KEY_SERVER);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", SP);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", "Juanito Tribalero Trakatero Chebichev");
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", CRLF);
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", CRLF);

    // body
    offset += snprintf(req_ctx->response + offset, BUFFER_LENGTH - offset, "%s", body);
    req_ctx->response[offset + 1] = '\0';

    return 0;
}

static httpmethod get_method_handler(const char *httpmethod)
{
    if (strcmp(httpmethod, "GET") == 0)
        return GET;
    return UNKNOWN;
}

static int handle_method(httpmethod method, request_ctx *req_ctx)
{
    if (method < UNKNOWN)
    {
        handlers[method](req_ctx);
    }
    return -1;
}

/*
TODO: Use struct request_parsed instead of **lines
*/
static int handle_request(char *response, char *lines[])
{

    if (lines[0] == NULL)
    {
        print_and_keep_going("Server", "Handle request");
        return -1;
    }

    printf("lines[0]: %s\n", lines[0]);

    // create duplicate of first line
    //(strtok operates on the same string)
    size_t fllenght = strlen(lines[0]);
    char *first_line = (char *)malloc(fllenght + 1);
    if (first_line == NULL)
    {
        print_and_keep_going("Server", "Handle request");
        return -1;
    }
    memcpy(first_line, lines[0], fllenght + 1);
    printf("first_line: %s\n", first_line);

    char *token = strtok(first_line, SP);

    /*
    need a function pointer to the right handle_method function
    */

    httpmethod method = get_method_handler(token);

    request_ctx req_ctx;
    req_ctx.response = response;
    char *resource = strtok(NULL, SP);
    req_ctx.resource = resource;
    char *http_version = strtok(NULL, SP);
    req_ctx.http_version = http_version;

    int whappend = handle_method(method, &req_ctx);

    free(first_line);

    return whappend;
}

void handle_child(int server_fd, server_ctx *server)
{
    pid_t pid = fork();
    if (pid > 0)
    {
        return;
    }

    // handle client connection
    ssize_t value_read;
    char buffer_stream[BUFFER_LENGTH] = {0};
    int client_conn;

    // well, I undertand (may be wrong) that accept() waits for a conn, creates a new socket if there's any
    // and then returns a fd to that new socket to communicate to the client.
    if ((client_conn = accept(server_fd, (struct sockaddr *)server->address, &server->addr_len)) < 0)
    {
        print_and_keep_going("Server child", "Error accepting connection...\n");
        return;
    };

    // TODO: use shutdown()-drain-close() technique
    value_read = read(client_conn, buffer_stream, BUFFER_LENGTH - 1); //-1 because EOF

    if (value_read == -1)
    {
        print_and_keep_going("Server child", "Error reading client request...\n");
        return;
    }

    // printf("Client request:\n");
    // char *lines[MAX_HEADERS_LINES_REQUEST] = {0};
    // tokenization_by_crlf(buffer_stream, strlen(buffer_stream), lines, MAX_HEADERS_LINES_REQUEST);
    // printf("lines[0] in handle_child(): %s\n", lines[0]);

    printf("Using other tokenizer function:\n");
    request_parsed req_parsed;
    parse_request(buffer_stream, strlen(buffer_stream), &req_parsed);
    printf("req_parsed->request_line: %s\n", req_parsed.request_line);
    printf("req_parsed->body: %s\n", req_parsed.body);

    char response[BUFFER_LENGTH];

    // TODO: Use struct request_parsed instead of **lines
    handle_request(response, lines); // gotta pass size of array

    send(client_conn, response, sizeof response, 0);
    printf("Msg sent\n");
    printf("=============================================\n");

    close(client_conn);
    kill_child();
}
