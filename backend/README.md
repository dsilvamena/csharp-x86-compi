# Backend de compilación

El compilador C++ es la fuente de verdad. El puente recibe C#, ejecuta
`compiler --json` por stdin y devuelve tokens, AST, ensamblador, mapa de líneas,
optimizaciones, diagnósticos y métricas. No ensambla ni ejecuta el programa generado:
la ejecución de la app ocurre en el simulador JavaScript del navegador.

## Desarrollo local

Desde la raíz, ejecuta `make -C c++`, luego:

```bash
cd backend
npm install
npm start
```

Express escucha en `http://localhost:8787`. Vite redirige `/api` a ese puerto.

- `POST /api/compile`: recibe `{ "source": "..." }` o texto plano y devuelve el
  contrato JSON del compilador. Los errores del programa devuelven HTTP 200 con
  `success: false`; los fallos del servicio usan códigos HTTP 4xx/5xx.
- `GET /api/health`: comprobación del servidor local.
- `/api/run` se retiró; no existe ejecución con Docker en el servicio web.

## Producción en Vercel

`api/compile.js` usa el mismo handler de `backend/compiler.js`, sin levantar
Express. El build genera `c++/build/vercel/compiler` para Linux y lo incluye
como archivo privado de la función. La API conserva la ruta `/api/compile`.

Consulta [la guía de despliegue](../docs/vercel.md).

## Variables

| Variable | Valor predeterminado | Uso |
|---|---|---|
| `PORT` | `8787` | Puerto de Express local |
| `COMPILER_BIN` | Binario local o de Vercel según entorno | Override opcional; no configurarlo en Vercel |
| `COMPILE_TIMEOUT_MS` | `15000` | Timeout de compilación, limitado a 20000 ms |

La función admite hasta 256 KB de fuente UTF-8 y 3 MB de salida del compilador.
El proceso hijo recibe el código por stdin; no usa un shell ni ejecuta el
ensamblador recibido. No se escriben archivos durante las solicitudes.

## Validación

Desde la raíz: `npm run test:api`. Recorre los fixtures reales y prueba errores de
entrada, fallos de arranque, salida inválida, salida excesiva y timeout a través
del entry point de Vercel.
