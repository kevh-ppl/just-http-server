#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "parser.h"
#include "standard.h"

void tokenization_by_crlf(const char *request, size_t req_str_len, char *lines[], int maxTokens)
{

    char request_cpy[1024] = {0};
    ssize_t nbytes_to_cpy = req_str_len < sizeof(request_cpy) - 1 ? req_str_len : sizeof(req_str_len) - 1;
    memcpy(request_cpy, request, nbytes_to_cpy);

    char *token = strtok(request_cpy, CRLF);
    int index = 0;
    while (token != NULL && index < maxTokens - 1) // -1 because gotta do a on purpose NULL pointer at the end
    {
        if (strcmp(token, CRLF) == 0) // ignore body for now
            break;

        lines[index] = token;
        index++;
        token = strtok(NULL, CRLF);
    }
    lines[index] = NULL;
    printf("First line of request:\n\t%s\n", lines[0]);
}

void parse_request(const char *request, size_t req_str_len, request_parsed *req_parsed)
{
    char request_cpy[BUFFER_LENGTH] = {0};
    ssize_t nbytes_to_cpy = req_str_len < sizeof(request_cpy) - 1 ? req_str_len : sizeof(req_str_len) - 1;
    memcpy(request_cpy, request, nbytes_to_cpy);

    char *first_line_token = strtok(request_cpy, SP);
    req_parsed->method = first_line_token;
    first_line_token = strtok(NULL, SP);
    req_parsed->resource = first_line_token;
    first_line_token = strtok(NULL, SP);
    req_parsed->http_version = first_line_token;

    char *token = strtok(request_cpy, CRLF);
    token = strtok(NULL, CRLF);

    int index = 0;
    while (token != NULL && index < MAX_HEADERS_LINES_REQUEST - 1) // -1 because gotta do a on purpose NULL pointer at the end
    {
        if (strcmp(token, CRLF) == 0) // ignore body for now
            break;

        token = strtok(NULL, CRLF);
        req_parsed->headers[index] = token;
    }

    while (token != NULL)
    {
        token = strtok(NULL, CRLF);
        req_parsed->body = token;
    }
}