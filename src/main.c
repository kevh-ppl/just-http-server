#include <stdio.h>
#include <stdlib.h> //free
#include <unistd.h> //read
#include <string.h> //strlen
#include <sys/socket.h>

#include "utils.h"
#include "server.h"

int main()
{
    server_ctx server;
    // first, i gotta setup up server and run it
    // listen to client connection
    // accept connection
    // handle request
    //   parsing request
    //   createing response
    //   send response
    //   close connection

    int server_fd = setup_server(&server);
    if (server_fd == -1)
    {
        abort_server("Server", "Couldn't setup and run server");
    }

    fprintf(stderr, "You can use netcat or telnet to connect to the server. You must give an input and the server response.\n");
    fprintf(stderr, "'telnet localhost 1313' or 'nc localhost 1313'\n");

    // handle client connection
    int client_conn;
    ssize_t value_read;
    char buffer_stream[1024] = {0};

    if ((client_conn = accept(server_fd, (struct sockaddr *)server.address, &server.addr_len)) < 0)
    {
        print_and_keep_going("Server", "Error accepting connection...%s");
    };

    value_read = read(client_conn, buffer_stream, 1024 - 1); //-1 because EOF

    char *hello = "Hi, this thing works!\n";
    send(client_conn, hello, strlen(hello), 0);
    printf("Msg sent\n");

    close(client_conn);

    close(server_fd);
}