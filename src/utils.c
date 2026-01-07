#include <stdio.h>
#include <stdarg.h> //va
#include <string.h> //strlen
#include <unistd.h> //_error
#include <stdlib.h>

void abort_server(const char *entity, const char *format, ...)
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

void print_and_keep_going(const char *entity, const char *format, ...)
{
    va_list args;

    if (format && *format && entity && *entity)
    {
        va_start(args, format);
        fprintf(stderr, "%s: ", entity);
        vfprintf(stderr, format, args);
        size_t len = strlen(format);
        if (format[len - 1] != '\n')
            fprintf(stderr, "\n");

        va_end(args);
    }
}

void do_fork()
{
    switch (fork()) // Create child process with unique PID and GID (parent ID is parent PID, server PID)
    {
    case -1:
        print_and_keep_going("Server", "Error creating child process to handle request");
    case 0:
        break; // child process
    default:
        _exit(EXIT_SUCCESS);
    }

    // At this point we are executing as the child process
}

void kill_child()
{
    // with_exit() does weird stuff
    // prints cli prompt between requests
    // may be because _exit() does not clean output streams,
    // just fd that belongs to the process being terminated
    exit(EXIT_SUCCESS);
}