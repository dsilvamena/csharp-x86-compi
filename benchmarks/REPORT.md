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
| `gcc-O0` | `gcc -O0` | referencia sin optimizar |
| `gcc-O2` | `gcc -O2` | referencia optimizada |
| `clang-O2` | `clang -O2` | segundo compilador comercial |

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
checksum que depende de todo el cómputo, de modo que `gcc -O2`/`clang -O2` no
pueden reducir el bucle a una constante. `hashloop` usa una recurrencia
multiplicativa precisamente para impedir que se cierre en forma analítica.

## 3. Resultados

Tiempos en milisegundos (mejor de 3 compilaciones / 5 ejecuciones). Salidas
verificadas idénticas en las 5 configuraciones de cada benchmark.

### fib — `Fib(35)` = 9227465

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 111.97 | 94.70 | 89 | 15792 |
| mine-opt | 113.37 | 94.56 | 83 | 15792 |
| gcc-O0 | 196.53 | 55.32 | 70 | 15776 |
| gcc-O2 | 347.94 | 24.00 | 397 | 15776 |
| clang-O2 | 654.89 | 36.90 | 78 | 16088 |

### hashloop — 10⁸ iteraciones = -5768770193658875776

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 112.40 | 317.54 | 68 | 15728 |
| mine-opt | 112.04 | 303.26 | 65 | 15728 |
| gcc-O0 | 197.87 | 107.03 | 47 | 15760 |
| gcc-O2 | 287.43 | 81.02 | 38 | 15760 |
| clang-O2 | 657.07 | 26.00 | 51 | 16064 |

### sieve — primos < 2·10⁶ = 148933

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 112.63 | 44.03 | 162 | 15784 |
| mine-opt | 112.67 | 44.13 | 159 | 15784 |
| gcc-O0 | 207.15 | 33.79 | 91 | 15856 |
| gcc-O2 | 311.52 | 22.74 | 61 | 15856 |
| clang-O2 | 707.65 | 22.79 | 82 | 16160 |

### matmul — `c[199,199]` = 333300

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 112.95 | 39.15 | 364 | 15784 |
| mine-opt | 113.43 | 38.90 | 361 | 15784 |
| gcc-O0 | 212.49 | 29.17 | 208 | 15856 |
| gcc-O2 | 346.71 | 16.55 | 226 | 15904 |
| clang-O2 | 763.68 | 16.69 | 222 | 16208 |

*(Datos crudos reproducibles en `benchmarks/results/results.csv`; tabla
autogenerada en `benchmarks/results/results.md`.)*

## 4. Análisis

**Rendimiento de ejecución (lo esencial).** Tomando `mine-opt` como referencia:

| Benchmark | mine-opt vs gcc-O0 | mine-opt vs gcc-O2 | mine-opt vs clang-O2 |
|---|---|---|---|
| fib | 1.7× más lento | 3.9× más lento | 2.6× más lento |
| hashloop | 2.8× más lento | 3.7× más lento | 11.7× más lento |
| sieve | 1.3× más lento | 1.9× más lento | 1.9× más lento |
| matmul | 1.3× más lento | 2.4× más lento | 2.3× más lento |

El compilador genera código **correcto y funcional**, con un costo típico de
**2–4× frente a `gcc -O2`** y a menudo **cercano a `gcc -O0`**. Las causas del
diferencial son las esperables de un generador de código simple:

- **Sin asignación de registros global.** El compilador usa un esquema de
  pila/acumulador: casi cada operación toca memoria. GCC/Clang mantienen las
  variables calientes (`i`, `h`, acumuladores) en registros. Esto explica el
  peor caso, `hashloop` (bucle apretado sobre una variable): Clang además aplica
  *strength reduction* y desenrollado y llega a ser **11.7×** más rápido.
- **Selección de instrucciones ingenua** y ausencia de LICM, desenrollado de
  bucles y vectorización.

**Efecto de `--opt` (optimización propia).** La diferencia `mine-opt` vs
`mine-noopt` en tiempo de ejecución es **marginal** (dentro del ruido: fib
94.56 vs 94.70 ms; sieve 44.13 vs 44.03 ms). El efecto visible está en el
**tamaño del ensamblador**, que el peephole reduce de forma consistente
(fib 89→83, hashloop 68→65, sieve 162→159, matmul 364→361 líneas). Interpretación
honesta: las optimizaciones implementadas (plegado de constantes, Sethi-Ullman,
peephole) son **locales**; no atacan el cuello de botella real —tráfico a
memoria por falta de asignación de registros— por lo que su impacto en tiempo es
pequeño en estos núcleos. Para exhibir mejoras de tiempo medibles con `--opt`
convendría un benchmark saturado de subexpresiones constantes (donde el plegado
sí dispara) o implementar asignación de registros.

**Tiempo de compilación.** Aquí este compilador aparece **más rápido**
(~112 ms constante) que `gcc -O0` (~200 ms), `gcc -O2` (~290–350 ms) y
`clang -O2` (~650–770 ms). La lectura correcta es matizada: (a) el lenguaje
aceptado es un subconjunto mínimo, sin el frontend ni el pipeline de
optimización de un compilador industrial; (b) esos ~112 ms están **dominados por
el paso `g++` de ensamblado y enlace**, no por el frontend propio, que es casi
instantáneo. Es decir, *compilar menos y optimizar menos* cuesta menos tiempo,
como es de esperar. Aun así muestra un punto real: un pipeline simple tiene
baja latencia de compilación.

**Tamaño de binario y ensamblador.** Los ejecutables son casi idénticos en
tamaño (~15.8 KB; domina el runtime de C enlazado estáticamente en la sección
relevante). En **líneas de ensamblador**, `gcc -O0`/`clang -O2` suelen ser más
compactos que `mine` (p. ej. sieve 61–91 vs 159); la excepción es `gcc -O2` en
`fib` (397 líneas) por *inlining* agresivo de la recursión, que cambia código por
velocidad.

**Hallazgo semántico: `int` de 64 bits.** El benchmark `hashloop` reveló que
este compilador implementa `int` como entero de **64 bits**, a diferencia de C#
y de C (32 bits). Con desbordamiento a 32 bits la salida era `-1206259584`; con
64 bits es `-5768770193658875776`. Para que la comparación fuera equivalente, la
versión en C de `hashloop` se ajustó a `long long`. Los otros tres benchmarks no
se ven afectados porque sus valores caben en 32 bits. *(Si el objetivo fuera la
conformidad estricta con C#, `int` debería ser de 32 bits.)*

## 5. Conclusiones

- El compilador produce código **correcto** para los cuatro núcleos: salida
  idéntica a GCC y Clang en las cinco configuraciones.
- El costo en tiempo de ejecución es de **2–4× frente a `gcc -O2`**, competitivo
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
