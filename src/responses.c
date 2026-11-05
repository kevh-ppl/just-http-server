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

static int get_fd_resource(char* path) {
  struct stat stbuf;
  int fd_body = -1;
  printf("%s\n", path);
  if (stat(path, &stbuf) == 0 && S_ISREG(stbuf.st_mode)) {
    // GET FD
    fd_body = open(path, O_RDONLY);

  } else {
    int stat_errno = errno;
    print_and_keep_going("Server", "Error %d doing stat for error 404 page",
                         stat_errno);
  }
  return fd_body;
}

/*
 * Never use without get_fd_resource
 * Returning 0 means that the file does not exists or there was an error.
 * Not the better solution.
 */
static ssize_t get_resource_size(char* path) {
  struct stat stbuf;
  if (stat(path, &stbuf) == -1) {
    print_and_keep_going("Server", "Error %d doing stat for %s", errno, path);
    return 0;
  }

  return stbuf.st_size;
}

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

  int fd_body = -1;
  if (strcmp(code, CODE_NOT_FOUND) == 0) {
    // add header "Close"
    offset = snprintf(header + header_len, BUFFER_LENGTH - header_len,
                      KEY_CONNECTION SP CONNECTION_CLOSE CRLF);

    header_len += offset;

    // need to reed 404.html harcoded error page.
    fd_body = get_fd_resource(BASE_PATH_WWW "/" ERROR_404_FILE);

    if (fd_body == -1) {
      print_and_keep_going("Server", "Error %d opening error 404 page");
    }

    body_len = get_resource_size(BASE_PATH_WWW "/" ERROR_404_FILE);
  }

  if (strcmp(code, CODE_FORBIDDEN) == 0) {
    // add header "Close"
    offset = snprintf(header + header_len, BUFFER_LENGTH - header_len,
                      KEY_CONNECTION SP CONNECTION_CLOSE CRLF);

    header_len += offset;

    // need to reed 404.html harcoded error page.
    fd_body = get_fd_resource(BASE_PATH_WWW "/" ERROR_403_FILE);

    if (fd_body == -1) {
      print_and_keep_going("Server", "Error %d opening error 403 page");
    }

    body_len = get_resource_size(BASE_PATH_WWW "/" ERROR_403_FILE);
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
