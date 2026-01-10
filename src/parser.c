#include <stdio.h>
#include <string.h>
#include <unistd.h>

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