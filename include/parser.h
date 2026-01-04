#ifndef PARSER_H
#define PARSER_H

typedef struct header_token
{
    char key[50];
    char value[50];
} header_token;

void tokenization_by_crlf(const char *request, size_t req_str_len);

#endif