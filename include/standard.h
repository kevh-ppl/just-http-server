#ifndef STANDARD_H
#define STANDARD_H

#define CR "\r"
#define LF "\n"
#define CRLF "\r\n"
#define SP " "

// #define GET "GET"
#define POST "POST"

typedef enum
{
        GET,
        UNKNOWN
} httpmethod;

#define CODE_OK "200"
#define STATUS_OK "OK"

// headers
#define KEY_CONTENT_TYPE "Content-Type:"
#define KEY_CONTENT_LENGHT "Content-Length:"
#define KEY_SERVER "Server:"

// content-types
#define VALUE_CONTENT_TYPE_TEXT "text/"

/*
The pieces of informations in this file quoted as comments are from
the RFC's 2616, 7231, 9110
*/

/*
HTTP-message   = Request | Response     ; HTTP/1.1 messages
generic-message = start-line
                          *(message-header CRLF)
                          CRLF
                          [ message-body ]
        start-line      = Request-Line | Status-Line
*/

/*
   Request       = Request-Line              ; Section 5.1
                           *( general-header         ; Section 4.5
                            | request-header         ; Section 5.3
                            | entity-header )        ; Section 7.1
                           CRLF
                           [ message-body ]          ; Section 7.2

 request-header = Accept                   ; Section 14.1
                      | Accept-Charset           ; Section 14.2
                      | Accept-Encoding          ; Section 14.3
                      | Accept-Language          ; Section 14.4
                      | Authorization            ; Section 14.8
                      | Expect                   ; Section 14.20
                      | From                     ; Section 14.22
                      | Host                     ; Section 14.23
                      | If-Match                 ; Section 14.24
*/

/*
Response      =        Status-Line               ; Section 6.1
                       *(( general-header        ; Section 4.5
                        | response-header        ; Section 6.2
                        | entity-header ) CRLF)  ; Section 7.1
                       CRLF
                       [ message-body ]          ; Section 7.2

Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF

response-header = Accept-Ranges           ; Section 14.5
                       | Age                     ; Section 14.6
                       | ETag                    ; Section 14.19
                       | Location                ; Section 14.30
                       | Proxy-Authenticate      ; Section 14.33
                       | Retry-After             ; Section 14.37
                       | Server                  ; Section 14.38
                       | Vary                    ; Section 14.44
                       | WWW-Authenticate        ; Section 14.47
*/

/*
entity-header  = Allow                    ; Section 14.7
               | Content-Encoding         ; Section 14.11
               | Content-Language         ; Section 14.12
               | Content-Length           ; Section 14.13
               | Content-Location         ; Section 14.14
               | Content-MD5              ; Section 14.15
               | Content-Range            ; Section 14.16
               | Content-Type             ; Section 14.17
               | Expires                  ; Section 14.21
               | Last-Modified            ; Section 14.29
               | extension-header
*/

#endif