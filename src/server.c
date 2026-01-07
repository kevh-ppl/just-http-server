#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> //stderror(), strlen()
#include <errno.h>  //errno
#include <netinet/in.h>
#include <sys/socket.h>

#include "parser.h"
#include "utils.h"
#include "server.h"

// #include <winsock2.h> // for Windows

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

void handle_child(int server_fd, server_ctx *server)
{
    do_fork();

    // handle client connection
    ssize_t value_read;
    char buffer_stream[1024] = {0};
    int client_conn;

    // well, I undertand (may be wrong) that accept() waits for a conn, creates a new socket if there's any
    // and then returns a fd to that new socket to communicate to the client.
    if ((client_conn = accept(server_fd, (struct sockaddr *)server->address, &server->addr_len)) < 0)
    {
        print_and_keep_going("Server child", "Error accepting connection...%s");
    };

    value_read = read(client_conn, buffer_stream, 1024 - 1); //-1 because EOF
    if (value_read == -1)
    {
        print_and_keep_going("Server child", "Error reading client request");
        return;
    }

    printf("Client request:\n");
    printf("%s\n", buffer_stream);
    tokenization_by_crlf(buffer_stream, strlen(buffer_stream));

    char *hello = "Hi, this thing works!\n";
    send(client_conn, hello, strlen(hello), 0);
    printf("Msg sent\n");

    close(client_conn);
    kill_child();
}