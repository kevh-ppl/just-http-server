#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "parser.h"
#include "standard.h"
#include "utils.h"

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

/*
 * First tokenization by '\r\n\r\n',
 * so Body is separated from the rest of the request.
 * Second tokenization by '\r\n',
 * so we separate each header.
 * Third tokenization by ' ', so we separate Method, Resource and Version.
 *
 */
int parse_request(request_parsed *req_parsed)
{
    // body is 4 memory addresses ahead of the pointer that strstr return
    char *headers_separates_body = strstr(req_parsed->raw, "\r\n\r\n"); // CRLFCRLF

    if (headers_separates_body == NULL)
    {
        // return 400;
        print_and_keep_going("Parser", "Error 400 Bad Request");
        return -1;
    }

    char *body = headers_separates_body + 4;
    req_parsed->body = body;

    // request line
    char *p = req_parsed->raw;
    char *q;
    q = strstr(p, "\r\n");
    if (q == NULL)
    {
        // return 400;
        print_and_keep_going("Parser", "Error 400 Bad Request");
        return -1;
    }
    *q = '\0';

    char *request_line = p;

    p = q + 2;

    char *save;
    req_parsed->method = strtok_r(request_line, SP, &save);
    if (req_parsed->method == NULL)
    {
        // return 400;
        print_and_keep_going("Parser", "Error 400 Bad Request");
        return -1;
    }

    req_parsed->resource = strtok_r(NULL, SP, &save);
    if (req_parsed->resource == NULL)
    {
        // return 400;
        print_and_keep_going("Parser", "Error 400 Bad Request");
        return -1;
    }

    req_parsed->http_version = strtok_r(NULL, SP, &save);
    if (req_parsed->http_version == NULL)
    {
        // return 400;
        print_and_keep_going("Parser", "Error 400 Bad Request");
        return -1;
    }

    // request line

    int n = 0;
    while (p < headers_separates_body && n < MAX_HEADERS_LINES_REQUEST)
    {
        q = strstr(p, "\r\n");
        if (q == NULL)
            break;
        *q = '\0';

        req_parsed->headers[n++] = p;
        p = q + 2;
    }

      return 0;
}