#ifndef PARSER_H
#define PARSER_H

#define BUFFER_LENGTH 8192 // redefined from server.h, i'll move it to utils.h i think
#define MAX_HEADERS_LINES_REQUEST 50
// typedef struct header_token
// {
//     char key[50];
//     char value[50];
// } header_token;

typedef struct request_parsed
{
    char raw[BUFFER_LENGTH];
    char *method;
    char *resource;
    char *http_version;
    char *body;
    char *headers[MAX_HEADERS_LINES_REQUEST];
} request_parsed;

int parse_request(request_parsed *req_parsed);

#endif