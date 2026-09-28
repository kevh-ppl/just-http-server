#include <fcntl.h>     //open
#include <inttypes.h>  //PRId64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "parser.h"
#include "reponses.h"
#include "server.h"
#include "standard.h"
#include "utils.h"

static int handle_get(request_parsed* req_p, response* out);
static handler_method_fn handlers[] = {
    [GET_EN] = handle_get,
    [UNKNOWN_EN] = NULL,
};

static int handle_get(request_parsed* req_p, response* out) {
  if (req_p->resource == NULL || req_p->http_version == NULL) {
    print_and_keep_going("Server",
                         "Either resource or http version not provided");
    return -1;
  }

  char* path_to_resource;
  if (strcmp(req_p->resource, "/") == 0) {
    int len_path =
        LEN_BASE_PATH_WWW + strlen(req_p->resource) + LEN_INDEX_FILE + 1;
    path_to_resource = (char*)malloc(len_path);
    if (path_to_resource == NULL) {
      print_and_keep_going("Server",
                           "Error allocating memory for path to resource");
      return -1;
    }
    memcpy(path_to_resource, BASE_PATH_WWW, LEN_BASE_PATH_WWW);
    snprintf(path_to_resource, len_path, "%s%s%s", BASE_PATH_WWW,
             req_p->resource, INDEX_FILE);
  } else {
    int len_path = LEN_BASE_PATH_WWW + strlen(req_p->resource) + 1;
    path_to_resource = (char*)malloc(len_path);
    if (path_to_resource == NULL) {
      print_and_keep_going("Server",
                           "Error allocating memory for path to resource");
      return -1;
    }
    memcpy(path_to_resource, BASE_PATH_WWW, LEN_BASE_PATH_WWW);
    snprintf(path_to_resource, len_path, "%s%s", BASE_PATH_WWW,
             req_p->resource);
  }

  // printf("path_ro_resource: %s\n", path_to_resource);

  struct stat stbuf;
  if (stat(path_to_resource, &stbuf) == -1) {
    print_and_keep_going("Server", "Error doing stat for path to resource");
    free(path_to_resource);
    return -1;
  }

  if (!S_ISREG(stbuf.st_mode)) {
    print_and_keep_going("Server", "Not a regular file");
    free(path_to_resource);
    return -1;
  }
  int fd_resource = open(path_to_resource, O_RDONLY);
  free(path_to_resource);
  if (fd_resource == -1) {
    print_and_keep_going("Server", "Error opening resource");
    return -1;
  }

  // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
  // then headers, an empty line and the body
  char header[BUFFER_LENGTH];
  int header_len = snprintf(header, sizeof header,
                            "%s" SP CODE_OK SP STATUS_OK CRLF KEY_CONTENT_TYPE
                                SP VALUE_CONTENT_TYPE_TEXT
                            "html; charset=utf-8" CRLF KEY_CONTENT_LENGHT SP
                            "%" PRId64 CRLF KEY_SERVER SP
                            "Juanito Tribalero Trakatero Chebichev" CRLF CRLF,
                            HTTP_VERSION, (int64_t)stbuf.st_size);
  if (header_len < 0 || (size_t)header_len >= sizeof header) {
    print_and_keep_going("Server", "Error building response headers");
    close(fd_resource);
    return -1;
  }

  // one allocation for headers + body, body is read right after the headers
  out->response = (char*)malloc(header_len + stbuf.st_size);
  if (out->response == NULL) {
    print_and_keep_going("Server", "Error allocating memory for response");
    close(fd_resource);
    return -1;
  }
  memcpy(out->response, header, header_len);

  ssize_t nbytes_body =
      read(fd_resource, out->response + header_len, stbuf.st_size);
  close(fd_resource);
  if (nbytes_body != stbuf.st_size) {
    print_and_keep_going("Server", "Error reading resource");
    return -1;
  }

  out->len = header_len + nbytes_body;
  return 0;
}

static httpmethod get_method_handler(char* httpmethod) {
  ////printf("Coco de aa: %s\n", httpmethod);
  if (httpmethod == NULL) return UNKNOWN_EN;  // 501 Not Inplemented
  if (strcmp(httpmethod, GET) == 0) return GET_EN;
  return UNKNOWN_EN;  // 501 Not Inplemented
}

static int handle_method(request_parsed* req_p, response* out) {
  httpmethod method = get_method_handler(req_p->method);
  if (method < UNKNOWN_EN) {
    return handlers[method](req_p, out);
  }
  return -1;  // TODO: 501 Not Implemented
}

int handle_request(request_parsed* req_p, response* out) {
  if (req_p == NULL || out == NULL) {
    print_and_keep_going("Server", "Handle request");
    return -1;
  }
  out->response = NULL;
  out->len = 0;
  return handle_method(req_p, out);
}

void free_response(response* r) {
  if (r == NULL) return;
  free(r->response);
  r->response = NULL;
  r->len = 0;
}

/*
 * TODO: COMPLETAR
 * Just sends the response string through the network
 * */
int send_response(char* response) { return 0; }
