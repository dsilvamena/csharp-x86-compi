# backend — puente Node/Express ↔ compilador C++

Puente delgado entre la app web (React) y el compilador C++. **No compila nada**:
hace `spawn(compiler, ['--json'])`, escribe el fuente C# por **stdin**, captura el
stdout bajo timeout, valida que sea JSON y lo reenvía tal cual.

## Arranque

```bash
cd backend
npm install
npm start                 # escucha en http://localhost:8787
```

Antes de arrancar, construye el compilador **del host** (no el de Docker):

```bash
cd ../c++ && make         # genera ./c++/build/compiler (binario nativo del host)
```

## Variables de entorno

| Var | Por defecto | Uso |
|---|---|---|
| `PORT` | `8787` | puerto HTTP |
| `COMPILER_BIN` | `../c++/build/compiler` | ruta al binario (útil si vive en otro sitio) |
| `COMPILE_TIMEOUT_MS` | `15000` | timeout por compilación |

## Endpoints

- `GET /api/health` → `{ ok, compiler }`.
- `POST /api/compile` (body `text/plain` o `{ "source": "..." }`) → el objeto JSON
  de `compiler --json`: `{ success, tokens, ast, asm, asmOpt, asmSourceMap, optReport, errors, metrics }`.
  El compilador sale 1 en error de compilación pero devuelve igual un JSON válido
  con `success:false`; el backend lo reenvía. `asmSourceMap` alinea cada línea de
  `asm` con la línea C# que la produjo, o `null`.
- `POST /api/run` (mismo body) → **ejecución real vía Docker** (opcional). Compila
  con `--json` y, si es válido, ensambla + ejecuta el `.s` dentro de `gcc:13`
  (`linux/amd64`) pasando el asm por **stdin** (`g++ -no-pie -x assembler -`, sin
  ficheros ni volúmenes) y devuelve `{ compiled, ok, output, stderr, exitCode,
  timedOut?, error? }`. Es **on-demand** (no en cada tecla) y requiere Docker
  corriendo. Vars: `RUN_IMAGE` (def. `gcc:13`), `RUN_TIMEOUT_SECS` (def. `5`,
  timeout del programa), `DOCKER_TIMEOUT_MS` (def. `30000`, ensamblado+arranque).

## Prueba rápida

```bash
curl -s localhost:8787/api/compile -H 'content-type: text/plain' \
  --data-binary @../c++/inputs/02_functions.cs | python3 -m json.tool
```

> El compilador genera **AT&T/Linux** (no corre nativo en macOS). La web muestra
> el `.s` siempre. La pestaña **Simulación** corre el `.s` en un simulador JS del
> frontend; la ejecución nativa real usa `/api/run` → Docker `gcc:13` y muestra
> el stdout real en "Salida".
