#include <stdio.h>
#include <stdarg.h> //va
#include <string.h> //strlen
#include <unistd.h> //_error
#include <stdlib.h>

void abort(const char *entity, const char *format, ...)
{
    va_list args;

    if (format && *format)
    { // format pointer and value pointed are not NULL
        va_start(args, format);
        fprintf(stderr, "%s: ", entity);
        vfprintf(stderr, format, args);
        size_t len = strlen(format);
        if (format[len - 1] != '\n')
            fprintf(stderr, "\n");
        va_end(args);
    }
    _exit(EXIT_FAILURE);
}