# just-http-server

Un servidor HTTP/1.1 mínimo escrito en C desde cero, sobre sockets POSIX.
Es un proyecto de aprendizaje: no usa ninguna librería externa, solo syscalls
(`socket`, `bind`, `listen`, `poll`, `accept`, `fork`, `read`, `send`).

Hoy sabe hacer una sola cosa: responder `GET` sirviendo archivos estáticos
desde el directorio [`www/`](www/).

## Requisitos

- Linux (usa `SO_REUSEPORT`, `poll`, `fork`)
- `gcc` y `make`
- `valgrind` (opcional, solo para el target `valgrid`)

## Compilar y ejecutar

```bash
make          # compila src/*.c -> ./server
./server      # escucha en el puerto 1313
```

Probar:

```bash
curl -v http://localhost:1313/            # sirve www/index.html
curl -v http://localhost:1313/index.html
nc localhost 1313                          # o telnet localhost 1313
```

Bajo valgrind:

```bash
make valgrid   # (sic) --track-origins=yes --show-leak-kinds=all
```

El binario `./server` está en `.gitignore`, no se commitea.

## Estructura

```
include/        cabeceras públicas
  server.h      API del servidor (setup_server, handle_child) y config
  parser.h      struct request_parsed y funciones de parseo
  standard.h    constantes del protocolo HTTP + citas de los RFC 2616/7231/9110
  utils.h       server_ctx, logging y helpers de proceso
src/
  main.c        arranque + bucle de poll()
  server.c      setup del socket, despacho por método, handler de GET
  parser.c      tokenización de la request
  utils.c       logging con varargs, fork/exit helpers
www/            document root que se sirve (index.html, favicon.ico)
Makefile
```

## Configuración

No hay flags ni variables de entorno todavía; todo se define con macros en
[include/server.h](include/server.h):

| Macro | Valor | Qué es |
|---|---|---|
| `SERVER_PORT` | `1313` | puerto de escucha |
| `BUFFER_LENGTH` | `8192` | tamaño del buffer de request y de response |
| `BASE_PATH_WWW` | `./www` | document root (relativo al CWD del proceso) |
| `INDEX_FILE` | `index.html` | archivo servido cuando la ruta es `/` |
| `HTTP_VERSION` | `HTTP/1.1` | versión anunciada en la status line |

Como `BASE_PATH_WWW` es relativo, hay que lanzar `./server` desde la raíz del
repo o no encontrará los archivos.

## Cómo entender el código

Ver [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) para el recorrido completo de
una petición, el modelo de concurrencia y las estructuras de datos.

Los bugs conocidos y lo que falta por hacer están en [TODO.md](TODO.md) — vale
la pena leerlo antes de tocar el parser.

## Licencia

GPL v2 — ver [LICENCE](LICENCE).
