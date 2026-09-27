#ifndef RESPONSES_H
#define RESPONSES_H

#include <stdio.h>

#include "parser.h"
#include "utils.h"

typedef struct response {
  char* response;
  ssize_t len;
} response;

typedef int (*handler_method_fn)(char* response, request_parsed* req_p);

int handle_request(char* response, request_parsed* req_parsed);

char* build_response(server_ctx server_ctx);

int send_response(char* response);

#endif
