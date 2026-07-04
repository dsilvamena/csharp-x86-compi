# Evaluación comparativa con compiladores comerciales

Comparación experimental de `csharp-x86-compi` frente a **GCC** y **Clang/LLVM**
sobre cuatro benchmarks. Cada programa se escribió dos veces con **algoritmo y
salida idénticos**: una vez en el subconjunto de C# que acepta este compilador
(`benchmarks/cs/*.cs`) y otra en C (`benchmarks/c/*.c`).

## 1. Metodología

**Configuraciones comparadas** (fuente → ejecutable nativo x86-64):

| Config | Herramienta | Notas |
|---|---|---|
| `mine-noopt` | este compilador `--no-opt` + `g++ -no-pie` (ensambla y enlaza) | sin optimizaciones |
| `mine-opt` | este compilador `--opt` + `g++ -no-pie` | plegado de constantes, Sethi-Ullman, peephole |
| `gcc-O0` | `gcc -O0 -fwrapv` | referencia sin optimizar, con wrap signed de `int32` |
| `gcc-O2` | `gcc -O2 -fwrapv` | referencia optimizada, con wrap signed de `int32` |
| `clang-O2` | `clang -O2 -fwrapv` | segundo compilador comercial, con wrap signed de `int32` |

**Qué se mide, por benchmark y configuración:**

- **Tiempo de compilación**: fuente → ejecutable, *mejor de 3* corridas.
- **Tiempo de ejecución**: del binario producido, *mejor de 5* corridas.
- **Líneas de ensamblador** generado (`.s`).
- **Tamaño del ejecutable** (bytes).
- **Salida del programa**: se verifica que las 5 configuraciones produzcan
  **exactamente la misma salida**; si no, el benchmark se marca inválido.

Se reporta el *mejor* tiempo (no el promedio) para reducir el ruido del
planificador del sistema operativo, práctica estándar en microbenchmarking.

**Entorno.** Contenedor Docker `gcc:13` (Debian, GCC 13.4.0, Clang instalado en
tiempo de corrida), plataforma `linux/amd64`, 15 CPUs.

> **Caveat importante — emulación.** El host es Apple Silicon (**arm64**); los
> binarios `linux/amd64` corren **emulados** (traducción x86→arm de Docker
> Desktop, `VirtualApple @ 2.50GHz`). Por tanto los tiempos **absolutos no
> representan hardware x86 nativo**. Sin embargo, **todas** las configuraciones
> se ejecutan bajo la *misma* emulación, así que las **comparaciones relativas
> (este compilador vs GCC vs Clang) siguen siendo válidas**. Para un reporte
> definitivo conviene repetir la suite en una máquina x86-64 Linux nativa; el
> harness es idéntico (`benchmarks/bench.sh`).

## 2. Benchmarks

| Nombre | Qué ejercita | Parámetro |
|---|---|---|
| `fib` | recursión, marcos de pila, overhead de llamada | `Fib(35)` ingenuo |
| `hashloop` | bucle aritmético apretado con dependencia de datos (`h = h*31 + i`) | 10⁸ iteraciones |
| `sieve` | arreglo 1D, saltos, accesos con paso no unitario | criba hasta 2·10⁶ |
| `matmul` | arreglos 2D, bucles anidados, multiplicación entera | matrices 200×200 |

Los cuatro son *resistentes a la eliminación de código muerto*: imprimen un
checksum que depende de todo el cómputo, de modo que `gcc -O2 -fwrapv`/
`clang -O2 -fwrapv` no puedan eliminar el bucle. `hashloop` usa una recurrencia
multiplicativa precisamente para impedir que se cierre en forma analítica. La
opción `-fwrapv` evita depender de comportamiento indefinido de C en el
desbordamiento signed y fija la misma semántica de `int32` que usa el backend.

## 3. Resultados

Tiempos en milisegundos (mejor de 3 compilaciones / 5 ejecuciones). Salidas
verificadas idénticas en las 5 configuraciones de cada benchmark.

### fib — `Fib(35)` = 9227465

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 111.48 | 94.10 | 89 | 15792 |
| mine-opt | 110.68 | 94.65 | 83 | 15792 |
| gcc-O0 | 196.95 | 55.02 | 70 | 15776 |
| gcc-O2 | 350.04 | 24.71 | 375 | 15776 |
| clang-O2 | 649.66 | 36.73 | 78 | 16088 |

### hashloop — 10⁸ iteraciones = -1206259584

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 112.33 | 427.74 | 68 | 15728 |
| mine-opt | 117.03 | 420.43 | 65 | 15728 |
| gcc-O0 | 199.05 | 129.46 | 47 | 15760 |
| gcc-O2 | 291.37 | 103.49 | 38 | 15760 |
| clang-O2 | 678.41 | 28.61 | 45 | 16064 |

### sieve — primos < 2·10⁶ = 148933

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 113.96 | 45.09 | 162 | 15784 |
| mine-opt | 115.50 | 43.86 | 159 | 15784 |
| gcc-O0 | 217.94 | 34.27 | 91 | 15856 |
| gcc-O2 | 314.97 | 23.04 | 67 | 15856 |
| clang-O2 | 707.62 | 23.38 | 82 | 16160 |

### matmul — `c[199,199]` = 333300

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 114.22 | 39.12 | 381 | 15784 |
| mine-opt | 114.17 | 39.52 | 378 | 15784 |
| gcc-O0 | 218.70 | 28.79 | 207 | 15856 |
| gcc-O2 | 374.12 | 16.42 | 226 | 15904 |
| clang-O2 | 766.76 | 16.59 | 222 | 16208 |

*(Datos crudos reproducibles en `benchmarks/results/results.csv`; tabla
autogenerada en `benchmarks/results/results.md`.)*

## 4. Análisis

**Rendimiento de ejecución (lo esencial).** Tomando `mine-opt` como referencia:

| Benchmark | mine-opt vs gcc-O0 | mine-opt vs gcc-O2 | mine-opt vs clang-O2 |
|---|---|---|---|
| fib | 1.7× más lento | 3.8× más lento | 2.6× más lento |
| hashloop | 3.3× más lento | 4.1× más lento | 14.7× más lento |
| sieve | 1.3× más lento | 1.9× más lento | 1.9× más lento |
| matmul | 1.4× más lento | 2.4× más lento | 2.4× más lento |

El compilador genera código **correcto y funcional**, con un costo típico de
**1.9–4.1× frente a `gcc -O2`** y a menudo **cercano a `gcc -O0`**. Las causas del
diferencial son las esperables de un generador de código simple:

- **Sin asignación de registros global.** El compilador usa un esquema de
  pila/acumulador: casi cada operación toca memoria. GCC/Clang mantienen las
  variables calientes (`i`, `h`, acumuladores) en registros. Esto explica el
  peor caso, `hashloop` (bucle apretado sobre una variable): Clang además aplica
  *strength reduction* y desenrollado y llega a ser **14.7×** más rápido.
- **Selección de instrucciones ingenua** y ausencia de LICM, desenrollado de
  bucles y vectorización.

**Efecto de `--opt` (optimización propia).** La diferencia `mine-opt` vs
`mine-noopt` en tiempo de ejecución es **marginal** (dentro del ruido: fib
94.65 vs 94.10 ms; sieve 43.86 vs 45.09 ms). El efecto visible está en el
**tamaño del ensamblador**, que el peephole reduce de forma consistente
(fib 89→83, hashloop 68→65, sieve 162→159, matmul 381→378 líneas). Interpretación
honesta: las optimizaciones implementadas (plegado de constantes, Sethi-Ullman,
peephole) son **locales**; no atacan el cuello de botella real —tráfico a
memoria por falta de asignación de registros— por lo que su impacto en tiempo es
pequeño en estos núcleos. Para exhibir mejoras de tiempo medibles con `--opt`
convendría un benchmark saturado de subexpresiones constantes (donde el plegado
sí dispara) o implementar asignación de registros.

**Tiempo de compilación.** Aquí este compilador aparece **más rápido**
(~111–117 ms) que `gcc -O0` (~197–219 ms), `gcc -O2` (~291–374 ms) y
`clang -O2` (~650–767 ms). La lectura correcta es matizada: (a) el lenguaje
aceptado es un subconjunto mínimo, sin el frontend ni el pipeline de
optimización de un compilador industrial; (b) esos tiempos están **dominados por
el paso `g++` de ensamblado y enlace**, no por el frontend propio, que es casi
instantáneo. Es decir, *compilar menos y optimizar menos* cuesta menos tiempo,
como es de esperar. Aun así muestra un punto real: un pipeline simple tiene
baja latencia de compilación.

**Tamaño de binario y ensamblador.** Los ejecutables son casi idénticos en
tamaño (~15.8 KB; domina el runtime de C enlazado estáticamente en la sección
relevante). En **líneas de ensamblador**, `gcc -O0`/`clang -O2` suelen ser más
compactos que `mine` (p. ej. sieve 67–91 vs 159); la excepción es `gcc -O2` en
`fib` (375 líneas) por *inlining* agresivo de la recursión, que cambia código por
velocidad.

**Semántica de enteros.** Esta versión del compilador implementa `int` como
entero de **32 bits**. Por eso las versiones C usan `int`, no `long long`. En
`hashloop` el desbordamiento signed es intencional para estresar aritmética
modular; las referencias GCC/Clang se compilan con `-fwrapv` para que esa
semántica sea explícita y comparable.

## 5. Conclusiones

- El compilador produce código **correcto** para los cuatro núcleos: salida
  idéntica a GCC y Clang en las cinco configuraciones.
- El costo en tiempo de ejecución es de **1.9–4.1× frente a `gcc -O2`**, competitivo
  con `gcc -O0`; el peor caso son los bucles muy apretados, donde la falta de
  asignación de registros pesa más.
- Las optimizaciones propias reducen el tamaño del ensamblador pero **casi no
  mueven el tiempo** en estos benchmarks; el margen real de mejora está en la
  asignación de registros y en optimizaciones de bucle.
- La comparación se ejecutó bajo emulación x86→arm; las conclusiones
  **relativas** son sólidas, pero para cifras absolutas debe repetirse en x86-64
  Linux nativo con el mismo `benchmarks/bench.sh`.

## 6. Reproducir

```bash
benchmarks/bench.sh      # macOS/Apple Silicon: usa Docker linux/amd64
```

Salidas en `benchmarks/results/`. Ver `benchmarks/README.md` para detalles.
