# TODO

Lo que falta y lo que está roto. Cada punto dice dónde está el problema para que
se pueda atacar sin releer todo el código; el recorrido de una petición está en
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Bugs y limitaciones conocidos

Vale la pena leerlo antes de tocar nada: varios de estos son la razón de
comportamientos raros.

**Protocolo**

- Solo `GET`. Si el método es desconocido, `handle_method()` no escribe nada en
  `response` y se envía el buffer de pila sin inicializar. No hay respuestas
  404, 501 ni 400: los errores solo se loguean y el cliente recibe basura.
- `Content-Type` está fijo a `text/html; charset=utf-8`, así que `favicon.ico`
  y cualquier binario se sirven con el tipo equivocado.
- Una petición = una conexión; no hay keep-alive ni pipelining.
- Se hace un único `read()` de hasta 8191 bytes: las peticiones más grandes o
  fragmentadas en varios segmentos TCP se parsean a medias.

**Memoria y corrección**

- `request_parsed` apunta a un buffer local de `parse_request()`, que deja de
  existir al volver. Que hoy funcione es casualidad del layout de la pila; hay
  que copiar los tokens (o parsear sobre el buffer del llamador).
- El bucle de cabeceras de `parse_request()` nunca incrementa `index`, así que
  todas las cabeceras van a parar a `headers[0]`.
- En el cálculo de bytes a copiar se usa `sizeof(req_str_len) - 1` (7, el tamaño
  del `size_t`) en la rama del `else`, en lugar del tamaño del buffer.
- El body se lee sin terminador y luego se formatea con `%s`, de modo que
  `snprintf` lee más allá del buffer; además `response[offset + 1] = '\0'` deja
  el byte en `offset` sin escribir.
- `send(client_conn, response, sizeof response, 0)` envía siempre 8192 bytes
  (el tamaño del array, no la longitud de la respuesta); debería usar `offset`.
- En las rutas de error de `handle_get()` se hace `return -1` sin cerrar
  `fd_resource` ni liberar `path_to_resource`/`body`.
- En los errores de `setup_server()` se hace `free(server)` sobre un struct que
  vive en la pila de `main()`; lo que hay que liberar es `server->address`.
- Zombis: el padre nunca hace `wait()`/`waitpid()` ni ignora `SIGCHLD`, así que
  cada hijo terminado se queda en la tabla de procesos (ver "Modelo de
  concurrencia" en [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)).

**Seguridad**

- No hay validación de path traversal: `GET /../..%2Fetc/passwd` se concatena
  tal cual a `./www` y puede salir del document root.
- No hay límites de tamaño, timeouts, ni rate limiting. No exponerlo a una red
  que no sea la local.

## Siguientes pasos

- [ ] Hacer que `request_parsed` sea dueño de sus strings.
- [ ] Respuestas de error reales (400 / 404 / 501) con un handler genérico.
- [ ] `Content-Type` por extensión.
- [ ] Normalizar la ruta y rechazar cualquier cosa que escape de `www/`.
- [ ] Enviar solo `offset` bytes y hacer el `send()` en bucle hasta vaciar.
- [ ] `signal(SIGCHLD, SIG_IGN)` o `waitpid(WNOHANG)` en el padre.
- [ ] Mover el `fork()` a después del `accept()`.
