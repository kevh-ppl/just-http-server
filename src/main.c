#include <stdio.h>
#include <stdlib.h> //free
#include <unistd.h> //read
#include <string.h> //strlen
#include <poll.h>   //instead of select()
#include <sys/socket.h>

#include "parser.h"
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

    // Trying to use poll syscall from Posix (there's another version from Linux kernel implementation called ppoll() btw)
    //  i need a loop right here and maybe use select syscall; turn out i'm using poll

    int theres_new_conn;
    nfds_t total_nfds;
    struct pollfd *pfds;

    total_nfds = 1;
    // gotta make an array for fds
    pfds = calloc(total_nfds, sizeof(struct pollfd));
    if (pfds == NULL)
        abort_server("Server", "Couldn't allocate memory for pfds\n");

    // i gotta put server_fd in pfds, not client_conn ahhh ._.
    // so when theres an incoming connections, an readiable event is issued and accept() can be used
    pfds[0].fd = server_fd;
    pfds[0].events = POLLIN;

    while (1)
    {
        memset(buffer_stream, 0, sizeof(buffer_stream));
        theres_new_conn = poll(pfds, total_nfds, -1);
        if (theres_new_conn == -1)
        {
            // theres an error on poll, can't recover
            abort_server("Server", "Error during poll");
        }
        else if (pfds->revents == 0) // no revents for any fds
        {
            continue;
        }

        // now i can handle the connection
        // well, I undertand (may be wrong) that accept() waits for a conn, creates a new socket if there's any
        // and then returns a fd to that new socket to communicate to the client.
        if ((client_conn = accept(server_fd, (struct sockaddr *)server.address, &server.addr_len)) < 0)
        {
            print_and_keep_going("Server", "Error accepting connection...%s");
        };

        value_read = read(client_conn, buffer_stream, 1024 - 1); //-1 because EOF
        if (value_read == -1)
        {
            print_and_keep_going("Server", "Error reading client request");
            continue;
        }

        printf("Client request:\n");
        printf("%s\n", buffer_stream);
        tokenization_by_crlf(buffer_stream, strlen(buffer_stream));

        char *hello = "Hi, this thing works!\n";
        send(client_conn, hello, strlen(hello), 0);
        printf("Msg sent\n");
        close(client_conn);
    }

    close(server_fd);
}