#ifndef RESPONSES_H
#define RESPONSES_H

#include <stdio.h>

#include "parser.h"

typedef struct response {
  char* response;
  ssize_t len;
} response;

typedef int (*handler_method_fn)(request_parsed* req_p, response* out);

// On success returns 0 and out->response is heap allocated (out->len bytes).
// The caller must always call free_response(), even on error.
int handle_request(request_parsed* req_p, response* out);

void free_response(response* r);

int send_response(char* response);

#endif
