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

static int handle_get(char* response, request_parsed* req_p);
static handler_method_fn handlers[] = {
    [GET_EN] = handle_get,
    [UNKNOWN_EN] = NULL,
};

static int handle_get(char* response, request_parsed* req_p) {
  if (req_p->resource == NULL || req_p->http_version == NULL) {
    print_and_keep_going("Server",
                         "Either resource or http version not provided");
    return -1;
  }
  // printf("Handling GET request with %s and %s\n", req_p->resource,
  // req_p->http_version);

  /*
  first i gotta check for the resource
  if it does not exists, return a 404 NOT FOUND response

  i can read first the file, see how many bytes it is,
  then allocate just that and read again

  first / gotta be replaced with BASE_PATH_WWW

  "./web" + resource -> Just gotta do a strcat
  */

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
    return -1;
  }

  if (!S_ISREG(stbuf.st_mode)) {
    print_and_keep_going("Server", "Not a regular file");
    return -1;
  }
  int fd_resource = open(path_to_resource, O_RDONLY);
  if (fd_resource == -1) {
    print_and_keep_going("Server", "Error opening resource");
    return -1;
  }

  char* body = (char*)malloc((int64_t)stbuf.st_size);
  // printf("st_size: %" PRId64 "\n", stbuf.st_size);
  if (body == NULL) {
    print_and_keep_going("Server", "Error allocating memory for body response");
    return -1;
  }

  int nbytes_body = read(fd_resource, body, (int64_t)stbuf.st_size);
  if (nbytes_body == -1) {
    print_and_keep_going("Server", "Error reading resource");
    return -1;
  }

  // printf("nbytes_read: %d\n", nbytes_body);
  //  at this point i can assamble the response with status 200
  //  Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

  int offset;
  offset = snprintf(response, BUFFER_LENGTH, "%s", HTTP_VERSION);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", SP);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", CODE_OK);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", SP);
  offset +=
      snprintf(response + offset, BUFFER_LENGTH - offset, "%s", STATUS_OK);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", CRLF);

  // headers
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s",
                     KEY_CONTENT_TYPE);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", SP);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s",
                     VALUE_CONTENT_TYPE_TEXT);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s",
                     "html; charset=utf-8");
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", CRLF);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s",
                     KEY_CONTENT_LENGHT);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", SP);
  offset +=
      snprintf(response + offset, BUFFER_LENGTH - offset, "%d", nbytes_body);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", CRLF);
  offset +=
      snprintf(response + offset, BUFFER_LENGTH - offset, "%s", KEY_SERVER);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", SP);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s",
                     "Juanito Tribalero Trakatero Chebichev");
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", CRLF);
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", CRLF);

  // body
  offset += snprintf(response + offset, BUFFER_LENGTH - offset, "%s", body);
  response[offset + 1] = '\0';

  free(path_to_resource);
  free(body);
  return 0;
}

static httpmethod get_method_handler(char* httpmethod) {
  ////printf("Coco de aa: %s\n", httpmethod);
  if (httpmethod == NULL) return UNKNOWN_EN;  // 501 Not Inplemented
  if (strcmp(httpmethod, GET) == 0) return GET_EN;
  return UNKNOWN_EN;  // 501 Not Inplemented
}

static int handle_method(char* response, request_parsed* req_p) {
  /*
  need a function pointer to the right handle_method function
  */
  httpmethod method = get_method_handler(req_p->method);
  if (method < UNKNOWN_EN) {
    handlers[method](response, req_p);
  }
  return -1;
}

int handle_request(char* response, request_parsed* req_parsed) {
  if (req_parsed == NULL || response == NULL) {
    print_and_keep_going("Server", "Handle request");
    return -1;
  }
  return handle_method(response, req_parsed);
}

/*
 * COMPLETAR
 *
 * */
char* build_response(server_ctx server_ctx) {
  char* response = NULL;

  return response;
}

/*
 * COMPLETAR
 * Just sends the response string through the network
 * */
int send_response(char* response) { return 0; }
