#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "parser.h"
#include "standard.h"
#include "utils.h"

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