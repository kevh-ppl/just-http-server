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
    char *request_line;
    char *body;
    char *headers[MAX_HEADERS_LINES_REQUEST];
} request_parsed;

void tokenization_by_crlf(const char *request, size_t req_str_len, char *lines[], int maxTokens);
void parse_request(const char *request, size_t req_str_len, request_parsed *req_parsed);

#endif