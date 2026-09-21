# Arquitectura

Documento para quien vaya a leer o modificar el código. Describe lo que el
servidor **hace hoy**, no lo que debería hacer.

## Modelo de concurrencia

Un proceso padre acepta eventos y un proceso hijo por conexión atiende la
petición ("fork-per-connection", pero con un giro: el `fork()` ocurre *antes*
del `accept()`).

```
main()                                   handle_child()            (hijo)
  setup_server() -> server_fd              fork()
  poll([server_fd], POLLIN, -1) <--+         |  padre: return
        |                          |         |  hijo:  accept()
        v                          |         v         read()
   revents & POLLIN ---------------+      parse_request()
        |                                    handle_request()
        v                                      -> handle_method()
   handle_child(server_fd, &server)              -> handlers[GET_EN]
                                               send() / close() / exit()
```

- [src/main.c](../src/main.c): crea el socket, registra `server_fd` en un array
  de `struct pollfd` de un solo elemento y hace `poll()` bloqueante infinito.
  Cuando hay `POLLIN` (conexión pendiente en la backlog queue) llama a
  `handle_child()`. El bucle es `while (1)`, así que el `free()`/`close()` del
  final de `main()` nunca se ejecutan.
- [src/server.c](../src/server.c) `handle_child()`: hace `fork()`. El padre
  vuelve de inmediato al `poll()`; el hijo es quien llama a `accept()`, lee la
  petición, responde, cierra y termina con `kill_child()` (`exit(EXIT_SUCCESS)`).

Dos consecuencias de que el `fork()` vaya antes del `accept()`:

1. El padre vuelve al `poll()` mientras la conexión sigue en la cola, así que
   `poll()` puede volver a reportar `POLLIN` por la *misma* conexión y forkear
   hijos de más, que se quedarán bloqueados en `accept()` esperando la siguiente.
2. El padre nunca hace `wait()`/`waitpid()` ni ignora `SIGCHLD`, así que cada
   hijo terminado queda como zombi.

## Setup del socket

`setup_server()` en [src/server.c](../src/server.c):

`socket(AF_INET, SOCK_STREAM, 0)` (TCP/IPv4) → `setsockopt(SO_REUSEADDR |
SO_REUSEPORT)` → `bind()` a `INADDR_ANY:SERVER_PORT` → `listen(fd, 3)`.
Devuelve el fd o `-1`; el llamador aborta con `abort_server()`.

El `struct sockaddr_in` vive en el heap y lo apunta `server_ctx` (ver
[include/utils.h](../include/utils.h)), que es lo único que se pasa entre
`main()` y el servidor.

Se probó `SOCK_NONBLOCK` y se descartó: como el `accept()` está en el hijo y es
el único punto de bloqueo deseado, el flag rompía el flujo.

## Parseo de la petición

[src/parser.c](../src/parser.c) rellena un `request_parsed`
([include/parser.h](../include/parser.h)):

```c
typedef struct request_parsed {
    char *method;        // "GET"
    char *resource;      // "/index.html"
    char *http_version;  // "HTTP/1.1"
    char *body;
    char *headers[MAX_HEADERS_LINES_REQUEST]; // 50
} request_parsed;
```

`parse_request()` copia el stream a un buffer local y lo trocea con `strtok()`:
primero por `SP` para la request line (método, recurso, versión) y luego por
`CRLF` para las cabeceras. Los campos del struct son **punteros al buffer local
de `parse_request()`**, no copias.

`tokenization_by_crlf()` es la versión anterior del troceador; ya no la llama
nadie, queda como referencia.

## Generación de la respuesta

`handle_request()` → `handle_method()` → tabla de punteros a función indexada
por el enum `httpmethod` de [include/standard.h](../include/standard.h):

```c
typedef int (*handler_method_fn)(char *response, request_parsed *req_p);

static handler_method_fn handlers[] = {
    [GET_EN]     = handle_get,
    [UNKNOWN_EN] = NULL,
};
```

Añadir un método nuevo (p. ej. `POST`) son tres pasos: una entrada en el enum
`httpmethod`, un `strcmp` en `get_method_handler()`, y la función
`handle_<método>` registrada en `handlers[]`.

`handle_get()`:

1. Construye la ruta: `"./www"` + recurso, y si el recurso es `/` le concatena
   `index.html`.
2. `stat()` sobre la ruta; exige que sea un archivo regular (`S_ISREG`).
3. `open()` + `read()` de `st_size` bytes a un buffer del heap.
4. Escribe la respuesta en el buffer `response` con `snprintf()` acumulando un
   `offset`, pieza a pieza: status line, `Content-Type`, `Content-Length`,
   `Server`, `CRLF CRLF`, body.

Las constantes del protocolo (`CRLF`, `SP`, `CODE_OK`, nombres de cabecera…)
están en [include/standard.h](../include/standard.h), junto con las gramáticas
de los RFC 2616 / 7231 / 9110 citadas en comentarios — útil como chuleta al
escribir handlers nuevos.

## Utilidades

[src/utils.c](../src/utils.c):

- `abort_server(entity, format, ...)`: log a `stderr` con prefijo y `_exit(EXIT_FAILURE)`.
- `print_and_keep_going(entity, format, ...)`: lo mismo pero sin salir. Es el
  mecanismo de error de todo el proyecto (no hay códigos de error propios).
- `do_fork()` / `kill_child()`: wrappers de `fork()` y `exit()`. `handle_child()`
  usa `fork()` directo, no `do_fork()`.

## Qué falta

Los bugs conocidos, las limitaciones del protocolo y la lista de siguientes
pasos están en [TODO.md](../TODO.md).
