#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "standard.h"

void tokenization_by_crlf(const char *request, size_t req_str_len)
{

    char request_cpy[1024] = {0};
    ssize_t nbytes_to_cpy = req_str_len < sizeof(request_cpy) - 1 ? req_str_len : sizeof(req_str_len) - 1;
    memcpy(request_cpy, request, nbytes_to_cpy);

    char *token = strtok(request_cpy, CRLF);
    while (token != NULL)
    {
        printf("Line: %s\n", token++);
        token = strtok(NULL, CRLF);
    }
}