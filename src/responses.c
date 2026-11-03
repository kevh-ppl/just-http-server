#include "responses.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
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
  int offset = snprintf(header, BUFFER_LENGTH,
                        "%s" SP "%s" SP
                        "%s" CRLF KEY_CONTENT_TYPE SP VALUE_CONTENT_TYPE_TEXT
                        "html; charset=utf-8" CRLF KEY_CONTENT_LENGHT SP
                        "%" PRId64 CRLF KEY_SERVER SP SERVER_NAME CRLF,
                        HTTP_VERSION, code, msg, (int64_t)0);

  int header_len = offset;

  if (offset < 0 || (size_t)header_len >= BUFFER_LENGTH) {
    print_and_keep_going("Server", "Error building response headers");
    /*
     * While is in our possibilities to send a response,
     * even a harcoded 500 one,
     * we must send it
     * */
    return -1;
  }

  if (strcmp(code, CODE_NOT_FOUND) == 0) {
    printf("It's 404\n");
    // add header "Close"
    offset = snprintf(header + header_len, BUFFER_LENGTH - header_len,
                      KEY_CONNECTION CONNECTION_CLOSE CRLF);
  }

  header_len += offset;

  if (offset < 0 || (size_t)header_len >= BUFFER_LENGTH) {
    print_and_keep_going("Server", "Error building response headers");
    return -1;
  }

  // add CRLF CRLF before body response
  offset = snprintf(header + header_len, BUFFER_LENGTH - header_len, CRLF CRLF);

  if (offset < 0 || (size_t)header_len >= BUFFER_LENGTH) {
    print_and_keep_going("Server", "Error building response headers");
    return -1;
  }

  header_len += offset;

  out->response = (char*)malloc(header_len);
  if (out->response == NULL) {
    print_and_keep_going("Server", "Error allocating memory for response");
    return -1;
  }

  memcpy(out->response, header, header_len);
  out->len = header_len;
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

int handle_200(response* out) {
  return build_error_response(out, CODE_OK, STATUS_OK);
}
