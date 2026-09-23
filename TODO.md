# TODO

Lo que falta y lo que está roto. Cada punto dice dónde está el problema para que
se pueda atacar sin releer todo el código; el recorrido de una petición está en
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Bugs y limitaciones conocidos

Vale la pena leerlo antes de tocar nada: varios de estos son la razón de
comportamientos raros.

**Protocolo**

- Solo `GET`. Si el método es desconocido, `handle_method()` no escribe nada en
  `response` y se envía el buffer de pila sin inicializar: un `POST` hoy
  devuelve 8 KB de ceros. No hay respuestas 501 ni 404. (Una petición mal
  formada sí se corta limpiamente: `parse_request()` devuelve `-1` y
  `handle_child` cierra la conexión, pero el cliente no recibe un 400.)
- `Content-Type` está fijo a `text/html; charset=utf-8`, así que `favicon.ico`
  y cualquier binario se sirven con el tipo equivocado.
- Una petición = una conexión; no hay keep-alive ni pipelining.
- Se hace un único `read()` de hasta 8191 bytes: las peticiones más grandes o
  fragmentadas en varios segmentos TCP se parsean a medias.

**Memoria y corrección**

- El body se lee sin terminador y luego se formatea con `%s`, de modo que
  `snprintf` lee más allá del buffer; además `response[offset + 1] = '\0'` deja
  el byte en `offset` sin escribir.
- `send(client_conn, response, sizeof response, 0)` envía siempre 8192 bytes
  (el tamaño del array, no la longitud de la respuesta); debería usar `offset`.
- En las rutas de error de `handle_get()` se hace `return -1` sin cerrar
  `fd_resource` ni liberar `path_to_resource`/`body`.
- En los errores de `setup_server()` se hace `free(server)` sobre un struct que
  vive en la pila de `main()`; lo que hay que liberar es `server->address`.

**Seguridad**

- No hay validación de path traversal: `GET /../..%2Fetc/passwd` se concatena
  tal cual a `./www` y puede salir del document root.
- No hay límites de tamaño, timeouts, ni rate limiting. No exponerlo a una red
  que no sea la local.

## Siguientes pasos

- [x] Hacer que `request_parsed` sea dueño de sus strings. (`raw[BUFFER_LENGTH]`
      dentro del struct, parseo con `strstr`/`strtok_r`, `parse_request()`
      devuelve `int`.)
- [ ] Respuestas de error reales (400 / 404 / 501) con un handler genérico.
- [ ] `Content-Type` por extensión.
- [ ] Normalizar la ruta y rechazar cualquier cosa que escape de `www/`.
- [ ] Enviar solo `offset` bytes y hacer el `send()` en bucle hasta vaciar.
- [x] `signal(SIGCHLD, SIG_IGN)` en el padre, para que el kernel recoja a los
      hijos y no queden zombis.
- [x] Mover el `fork()` a después del `accept()`. El padre cierra su copia de
      `client_conn` y el hijo cierra `server_fd`.
