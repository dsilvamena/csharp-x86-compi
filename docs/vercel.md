# Despliegue en Vercel

La web conserva tokens, AST, ensamblador, optimizaciones, errores y el simulador
con depuración por pasos. La ejecución nativa con Docker se retiró de la app.
Los scripts Docker del compilador C++ siguen disponibles para sus pruebas locales.

## Cómo funciona

```text
Editor C# → POST /api/compile → ejecutable C++ → JSON con ensamblador
                                                    ↓
                                    simulador JavaScript del navegador
```

En cada despliegue Vercel construye el compilador para Linux. El binario es un
archivo privado de la función Node.js; no se publica dentro de los assets web.
Durante una solicitud solo transforma el C# en ensamblador. El programa del usuario
se ejecuta en la CPU virtual del navegador.

## 1. Subir la rama

Los cambios están preparados en `feat/vercel-simulation`. Revisa el diff, crea un
commit de los archivos de esta implementación y sube la rama al repositorio remoto:

```bash
git push -u origin feat/vercel-simulation
```

No es necesario subir `node_modules`, `dist`, `.vercel` ni ningún binario de `c++/build`.
El lockfile de `typescript/package-lock.json` sí debe estar versionado.

## 2. Crear el proyecto

En Vercel entra a **Add New → Project** e importa el repositorio Git.
Usa estos valores:

| Opción | Valor |
|---|---|
| Root Directory | Raíz del repositorio (`.`), no `typescript` ni `backend` |
| Framework Preset | Other |
| Node.js Version | 24.x |
| Install Command | `dnf install -y libstdc++-static && npm --prefix typescript ci` |
| Build Command | `npm run build:vercel` |
| Output Directory | `typescript/packages/web/dist` |
| Environment Variables | Ninguna obligatoria |

Los comandos y el directorio de salida ya están definidos en `vercel.json`.
Conserva esos valores si Vercel los detecta automáticamente. El `package.json`
raíz declara Node.js 24.x. No configures `COMPILER_BIN`, `BACKEND_URL` ni una URL
pública de backend: frontend y API se sirven desde el mismo dominio.

Para probar esta rama puedes seleccionarla al importar el repositorio si la UI lo
permite. Si el proyecto ya existe y sigue `main`, subir esta rama genera un
**Preview Deployment**; también puedes crear un deployment eligiendo su referencia
Git en la pantalla del proyecto. Comprueba que el deployment indique
`feat/vercel-simulation` antes de revisar sus resultados.

## 3. Qué esperar del build

El build usa Amazon Linux 2023 y herramientas `g++`/`make`. La instalación añade
`libstdc++-static` para enlazar el soporte de C++ sin depender de la versión de
`libstdc++` disponible en ejecución.

`npm run build:vercel`:

1. Fuerza un build Linux nuevo en `c++/build/vercel/compiler`.
2. Ejecuta una compilación pequeña para comprobar que el binario responde JSON.
3. Construye el frontend con Vite.

`vercel.json` empaqueta el ejecutable mediante `includeFiles` y da a la función
un máximo de 30 segundos. El timeout interno de compilación es de 15 segundos.
No requiere Docker, base de datos ni un servidor Express permanente.

## 4. Comprobar el deployment

En la URL de preview:

1. El ejemplo inicial debe generar ensamblador en la pestaña x86.
2. Revisa Tokens, AST y Optimización.
3. Pulsa **Run**, luego **Ejecutar** en Simulación: Fibonacci debe mostrar `6765`.
4. Prueba los controles de pasos y los registros del depurador.
5. Selecciona otro ejemplo y confirma que recompila.
6. Introduce un error de C# y comprueba el diagnóstico.
7. Confirma que no hay botón de Docker.

También puedes comprobar la API con la URL real del deployment:

```bash
curl -i https://TU-PROYECTO.vercel.app/api/compile \
  -H 'Content-Type: application/json' \
  --data '{"source":"class Program { static int Main() { Console.WriteLine(5); return 0; } }"}'
```

Debe responder HTTP 200 con `success: true` y ensamblador. Las previews protegidas
pueden requerir iniciar sesión en Vercel para acceder desde el navegador; esa
protección también puede bloquear el ejemplo de curl.

## 5. Pasar a producción

Cuando la preview funcione, integra la rama en la rama de producción del proyecto
(normalmente `main`). Vercel realizará el despliegue de producción automáticamente.
Puedes añadir un dominio propio desde **Settings → Domains**.

La validación local y el build Linux no sustituyen la prueba en Vercel: revisa la
preview antes de dar por confirmado el empaquetado en su runtime real.

## Diagnóstico

- **Frontend funciona, compilación falla:** revisa los logs de la función
  `/api/compile`. Confirma que `includeFiles` está en `vercel.json` y Root Directory
  apunta a la raíz del repo. El binario no debe quedar en `dist`.
- **`libstdc++` o fallo al enlazar:** confirma que se ejecutó el Install Command
  completo y que el build usa Node.js 24 / Amazon Linux 2023.
- **`Exec format error`:** verifica que se ejecutó `build:vercel`; no subas el
  ejecutable de macOS. El build fuerza recompilar y usa una ruta distinta a la local.
- **HTTP 504:** el programa excedió el tiempo de compilación. Simplifica la entrada.
- **HTTP 413:** la entrada supera 256 KB o el JSON del compilador supera 3 MB.
- **Límite de pasos del simulador:** reduce iteraciones o tamaño del programa.

## Desarrollo y comprobaciones locales

Requiere Node.js 24, npm y un compilador C++17.

```bash
npm --prefix typescript ci
npm --prefix backend install
npm run typecheck
npm run build
npm run test:api
npm run test:sim
cd typescript/packages/web
npx playwright install chromium
cd ../../..
npm run test:e2e
```

Para usar la app localmente, inicia `npm --prefix backend start` y
`npm --prefix typescript run dev` en terminales separadas.

Referencias oficiales: [build image](https://vercel.com/docs/builds/build-image),
[Node.js Functions](https://vercel.com/docs/functions/runtimes/node-js) e
[includeFiles](https://vercel.com/docs/project-configuration/vercel-json#functions).

## Validación de esta implementación

- TypeScript y build de producción: correctos.
- API: 5 pruebas correctas, incluyendo todos los fixtures de compilación y los
  11 programas con salida de referencia en el simulador, con y sin optimización.
- Simulador: 8 pruebas unitarias correctas; la prueba antigua que depende de
  ensambladores preexistentes en `outputs` se omite si no existen. La comparación
  con los fixtures está cubierta directamente por las pruebas de API.
- Navegador: 12 pruebas Playwright correctas. Usan puertos propios (5175/8789),
  configurables mediante `E2E_WEB_PORT` y `E2E_BACKEND_PORT`.
- Linux x86-64: instalación limpia, build completo y las 5 pruebas de API correctas
  en Amazon Linux 2023 con Node.js 24 (imagen de runtime de Lambda).
- Pendiente: comprobar una preview en Vercel con el empaquetado de su plataforma.
