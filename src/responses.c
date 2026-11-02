#include "responses.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "methods.h"
#include "server.h"
#include "standard.h"
#include "utils.h"

static int build_error_response(response* out, char* code, char* msg) {
  if (out == NULL) {
    // send generic 500 resonse at the end of stack trace
    return 0;
  }

  char header[BUFFER_LENGTH];
  int header_len = snprintf(header, sizeof(header))

      return 0;
}

int handle_404(response* out) {
  return build_error_response(out, CODE_NOT_FOUND, STATUS_NOT_FOUND);
}

int handle_500(response* out) {
  return build_error_response(out, CODE_INTERNAL_SERVER_ERROR,
                              STATUS_INTERNAL_SERVER_ERROR);
}

int handle_403(response* out) {
  return build_error_response(out, CODE_FORBIDDEN, STATUS_FORBIDDEN);
}
