#include <stdio.h>
#include <stdlib.h> //free
#include <unistd.h> //read
#include <string.h> //strlen
#include <poll.h>   //instead of select()
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
        theres_new_conn = poll(pfds, total_nfds, -1);
        if (theres_new_conn == -1)
        {
            // theres an error on poll, can't recover
            abort_server("Server", "Error during poll");
        }
        else if (pfds->revents & POLLIN)
        {
            /*
            I didn't read well Linux Man Page about poll and pollfd
            I wasn't using bitwise AND operator and instead
            I was comparing with normal int values (pdfs->revents == 0)
            */
            // now i can handle the connection
            handle_child(server_fd, &server);
        }
    }

    free(server.address);
    close(server_fd);
}