# Arquitectura

Documento para quien vaya a leer o modificar el código. Describe lo que el
servidor **hace hoy**, no lo que debería hacer.

## Modelo de concurrencia

Un proceso hijo por conexión ("fork-per-connection"): el padre acepta y delega,
el hijo atiende la petición entera y muere.

```
main()                                   handle_child()            (hijo)
  signal(SIGCHLD, SIG_IGN)                 accept() -> client_conn
  setup_server() -> server_fd              fork()
  poll([server_fd], POLLIN, -1) <--+         |  padre: close(client_conn), return
        |                          |         |  hijo:  close(server_fd)
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
- [src/server.c](../src/server.c) `handle_child()`: acepta la conexión y **luego**
  hace `fork()`. El padre cierra su copia de `client_conn` y vuelve al `poll()`;
  el hijo cierra `server_fd`, lee, responde, y termina con `kill_child()`.

Dos detalles que no son opcionales en este modelo:

1. **Cada proceso cierra el fd que no le toca.** Tras el `fork()` ambos tienen
   una copia de los dos descriptores, y un socket no se cierra de verdad hasta
   que se cierran todas sus copias. Si el padre no soltara `client_conn`, le
   iría acumulando un fd por conexión y el cliente no vería el cierre limpio.
2. **`signal(SIGCHLD, SIG_IGN)`** en `main()`: le dice al kernel que no se
   guarde el estado de salida de los hijos, que es lo que evita los zombis.
   Se eligió sobre un handler con `waitpid()` porque un handler interrumpiría
   el `poll()` con `EINTR` cada vez que muere un hijo, y el bucle lo trata hoy
   como error fatal. Si algún día se añade cualquier handler, hay que tratar
   `EINTR` en [src/main.c](../src/main.c) antes que nada.

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
    char raw[BUFFER_LENGTH];  // copia de la petición; el struct es su dueño
    char *method;             // "GET"
    char *resource;           // "/index.html"
    char *http_version;       // "HTTP/1.1"
    char *body;
    char *headers[MAX_HEADERS_LINES_REQUEST]; // 50
} request_parsed;
```

**El struct es dueño de sus bytes.** `handle_child()` copia el buffer de `read()`
en `raw` (usando `value_read`, no `strlen`, para no cortar en un `'\0'`), y todos
los `char *` del struct apuntan dentro de `raw`. No hay asignaciones sueltas ni
nada que liberar: la vida de los campos es la vida del struct.

`parse_request(request_parsed *)` trocea `raw` en sitio, en este orden:

1. `strstr(raw, "\r\n\r\n")` localiza el fin de las cabeceras; `body` es eso + 4.
2. La primera línea se corta por su `\r\n` y se parte con `strtok_r` por espacios
   → `method`, `resource`, `http_version`.
3. El resto se recorre línea a línea con `strstr(p, "\r\n")`, escribiendo `'\0'`
   en cada terminador, hasta llegar al límite de las cabeceras o a las 50 líneas.

Devuelve `0`, o `-1` si la petición está mal formada (sin `\r\n\r\n`, o con una
request line incompleta). `handle_child()` comprueba ese retorno y cierra la
conexión sin responder — pendiente convertirlo en un 400, ver [TODO.md](../TODO.md).

Nota sobre `strtok`: el segundo argumento es un **conjunto de caracteres**, no una
secuencia, así que `strtok(s, "\r\n")` trocea por CR o LF indistintamente y
colapsa delimitadores consecutivos — con lo cual la línea en blanco que separa
cabeceras de body es invisible. Por eso el parseo por líneas usa `strstr` y no
`strtok`. Para partir la request line, donde el delimitador sí es un solo
carácter, se usa `strtok_r` (la variante reentrante, sin estado global).

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
