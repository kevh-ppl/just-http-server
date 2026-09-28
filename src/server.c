#include "server.h"

#include <errno.h>       //errno
#include <netinet/in.h>  //htons to flip endianess
#include <stdlib.h>
#include <string.h>  //stderror(), strlen()
#include <sys/socket.h>
#include <unistd.h>

#include "parser.h"
#include "reponses.h"
#include "utils.h"

// #include <winsock2.h> // for Windows

/*
    @brief Setup and initializes server

    Creates socket AF_INET (IPV_4) and SOCK_STREAM (TCP)
    @return Server FD or -1 on error
*/
int setup_server(server_ctx* server) {
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
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) <
      0)  // using SOCL_STREAM sets up our server to go with TCP
  {
    print_and_keep_going("Server", "socket failed");
    return -1;
  }

  // set opt for socket
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    free(server);
    print_and_keep_going("Server", "setsocketopt");
    return -1;
  }

  server->address->sin_family = AF_INET;          // ipv4
  server->address->sin_addr.s_addr = INADDR_ANY;  // any address
  server->address->sin_port =
      htons(SERVER_PORT);  // we must use htons to convert host endianess to
                           // network endianess (big-endian)

  // we had specified the port, now we must bind it
  //  I must to cast it to struct sockaddr*
  if (bind(server_fd, (struct sockaddr*)server->address, server->addr_len) <
      0) {
    free(server);
    print_and_keep_going("Serverss", "Error binding port: %s", strerror(errno));
    return -1;
  }

  // prepare to accpet conections
  // the second param is the conns that can be queued
  if (listen(server_fd, 3) < 0) {
    print_and_keep_going("Server", "shalalala, listening on port %d",
                         SERVER_PORT);
    return -1;
  }

  return server_fd;
}

static void init_request_parsed(request_parsed* req_p) {
  req_p->raw[0] = '\0';
  req_p->method = NULL;
  req_p->resource = NULL;
  req_p->http_version = NULL;
  req_p->body = NULL;

  for (int i = 0; i < MAX_HEADERS_LINES_REQUEST; i++) {
    req_p->headers[i] = NULL;
  }
}

void handle_child(int server_fd, server_ctx* server) {
  // well, I undertand (may be wrong) that accept() waits for a conn,
  // creates a new socket if there's any
  // and then returns a fd to that new socket to communicate to the client.
  int client_conn;
  if ((client_conn = accept(server_fd, (struct sockaddr*)server->address,
                            &server->addr_len)) < 0) {
    print_and_keep_going("Server child", "Error accepting connection...\n");
    return;
  };

  pid_t pid = fork();
  if (pid == -1) {
    print_and_keep_going("Server child", "Error forking process");
    close(client_conn);
    return;
  }

  if (pid > 0)  // parent process continues here
  {
    /*
    Parent needs to close the client connection so the child never becomes
    a zombie staying at the processes table.
    */
    close(client_conn);
    return;
  }

  // child does not need the listening connection
  close(server_fd);

  // handle client connection
  ssize_t value_read;
  char buffer_stream[BUFFER_LENGTH] = {0};

  // TODO: use shutdown()-drain-close() technique
  value_read =
      read(client_conn, buffer_stream, BUFFER_LENGTH - 1);  //-1 because EOF

  if (value_read == -1) {
    print_and_keep_going("Server child", "Error reading client request...\n");
    return;
  }

  // //printf("Client request:\n");
  // //printf("lines[0] in handle_child(): %s\n", lines[0]);

  // printf("Using other tokenizer function:\n");
  request_parsed req_parsed;
  init_request_parsed(&req_parsed);

  int n = value_read < BUFFER_LENGTH - 1 ? value_read : BUFFER_LENGTH - 1;
  memcpy(req_parsed.raw, buffer_stream, n);
  req_parsed.raw[n] = '\0';

  int result_parsing = parse_request(&req_parsed);
  if (result_parsing < 0) {
    close(client_conn);
    kill_child();
  }

  response res;
  if (handle_request(&req_parsed, &res) == 0) {
    send(client_conn, res.response, res.len, 0);
  }
  free_response(&res);
  // printf("Msg sent\n");
  // printf("=============================================\n");

  close(client_conn);
  kill_child();
}
