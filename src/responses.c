#include "responses.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "methods.h"
#include "server.h"
#include "standard.h"
#include "utils.h"

static int build_error_response(response* out, char* code, char* msg) {
  if (out == NULL) {
    // send generic 500 resonse at the end of stack trace
    return 0;
  }

  ssize_t body_len = 0;

  char header[BUFFER_LENGTH];
  int offset = snprintf(
      header, BUFFER_LENGTH,
      "%s" SP "%s" SP "%s" CRLF KEY_CONTENT_TYPE SP VALUE_CONTENT_TYPE_TEXT
      "html; charset=utf-8" CRLF KEY_SERVER SP SERVER_NAME CRLF,
      HTTP_VERSION, code, msg);

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

  int fd_body;
  if (strcmp(code, CODE_NOT_FOUND) == 0) {
    // add header "Close"
    offset = snprintf(header + header_len, BUFFER_LENGTH - header_len,
                      KEY_CONNECTION SP CONNECTION_CLOSE CRLF);

    header_len += offset;

    // need to reed 404.html harcoded error page.
    struct stat stbuf;
    printf(BASE_PATH_WWW "/" ERROR_404_FILE "\n");
    if (stat(BASE_PATH_WWW "/" ERROR_404_FILE, &stbuf) == 0) {
      if (S_ISREG(stbuf.st_mode)) {
        fd_body = open(BASE_PATH_WWW "/" ERROR_404_FILE, O_RDONLY);
        if (fd_body != -1) {
          body_len = stbuf.st_size;

        } else {
          print_and_keep_going("Server", "Error %d opening error 404 page");
        }
      } else {
        print_and_keep_going(
            "Server", "Error %d, not a regular file:  error 404 page", errno);
      }
    } else {
      int stat_errno = errno;
      print_and_keep_going("Server", "Error %d doing stat for error 404 page",
                           stat_errno);
    }
  }

  if (offset < 0 || (size_t)header_len >= BUFFER_LENGTH) {
    print_and_keep_going("Server", "Error building response headers");
    return -1;
  }

  if (offset < 0 || (size_t)header_len >= BUFFER_LENGTH) {
    print_and_keep_going("Server", "Error building response headers");
    return -1;
  }

  // add content lenght header
  offset = snprintf(header + header_len, BUFFER_LENGTH - header_len,
                    KEY_CONTENT_LENGHT SP "%" PRId64 CRLF, (int64_t)body_len);
  header_len += offset;

  // BEFORE memcpy the headers, i gotta make sure to add CRLF CRLF
  offset = snprintf(header + header_len, BUFFER_LENGTH - header_len, CRLF CRLF);
  header_len += offset;

  size_t response_len = (size_t)(header_len + body_len);
  out->response = (char*)malloc(response_len);
  if (out->response == NULL) {
    print_and_keep_going("Server", "Error allocating memory for response");
    return -1;
  }

  memcpy(out->response, header, header_len);

  if (body_len > 0) {
    ssize_t nbytes_body = read(fd_body, out->response + header_len, body_len);
    printf("nbytes_read: %zu\n", nbytes_body);
    if (nbytes_body != body_len) {
      print_and_keep_going("Server", "Error reading error 404 page file");
    }
  }

  printf("Response: \n%s\n", out->response);
  close(fd_body);

  out->len = response_len;
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
