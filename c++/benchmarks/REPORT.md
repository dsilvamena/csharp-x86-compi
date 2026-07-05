# Evaluación comparativa con compiladores comerciales

Comparación experimental de `csharp-x86-compi` frente a **GCC**, **Clang/LLVM**
y **.NET Native AOT** sobre cuatro benchmarks. Cada programa se escribió con
**algoritmo, salida y ancho de entero equivalentes**: una vez en el subconjunto
de C# que acepta este compilador (`benchmarks/cs/*.cs`), otra en C
(`benchmarks/c/*.c`) y otra en C# para .NET (`benchmarks/dotnet/*.cs`). Como el
`int` de este compilador se implementa como entero de 64 bits, las versiones C
usan tipos de 64 bits (`int64_t`/`uint64_t`) y las versiones .NET usan `long`
donde corresponde para modelar la misma semántica.

## 1. Metodología

**Configuraciones comparadas** (fuente → ejecutable nativo x86-64):

| Config | Herramienta | Notas |
|---|---|---|
| `mine-noopt` | este compilador `--no-opt` + `g++ -no-pie` (ensambla y enlaza) | sin optimizaciones |
| `mine-opt` | este compilador `--opt` + `g++ -no-pie` | plegado de constantes, Sethi-Ullman, peephole |
| `gcc-O0` | `gcc -O0` | referencia sin optimizar |
| `gcc-O2` | `gcc -O2` | referencia optimizada |
| `clang-O2` | `clang -O2` | segundo compilador comercial |
| `dotnet-aot` | `dotnet publish -c Release -p:PublishAot=true` | referencia C# comercial AOT |

**Qué se mide, por benchmark y configuración:**

- **Tiempo de compilación**: fuente → ejecutable, *mejor de 3* corridas.
- **Tiempo de ejecución**: del binario producido, *mejor de 5* corridas.
- **Líneas de ensamblador** generado (`.s`); para Native AOT se reporta `NA`
  porque el pipeline de `dotnet publish` no deja un `.s` comparable.
- **Tamaño del ejecutable** (bytes).
- **Salida del programa**: se verifica que las 6 configuraciones produzcan
  **exactamente la misma salida**; si no, el benchmark se marca inválido.

Se reporta el *mejor* tiempo (no el promedio) para reducir el ruido del
planificador del sistema operativo, práctica estándar en microbenchmarking.

**Entorno.** Contenedor Docker `mcr.microsoft.com/dotnet/sdk:8.0` (Debian 12,
toolchain nativo instalado en tiempo de corrida), plataforma `linux/amd64`,
15 CPUs.

> **Caveat importante — emulación.** El host es Apple Silicon (**arm64**); los
> binarios `linux/amd64` corren **emulados** (traducción x86→arm de Docker
> Desktop, `VirtualApple @ 2.50GHz`). Por tanto los tiempos **absolutos no
> representan hardware x86 nativo**. Sin embargo, **todas** las configuraciones
> se ejecutan bajo la *misma* emulación, así que las **comparaciones relativas
> (este compilador vs GCC vs Clang vs .NET Native AOT) siguen siendo válidas**.
> Para un reporte definitivo conviene repetir la suite en una máquina x86-64
> Linux nativa; el harness es idéntico (`benchmarks/bench.sh`).

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
verificadas idénticas en las 6 configuraciones de cada benchmark.

### fib — `Fib(35)` = 9227465

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 103.72 | 94.96 | 89 | 15832 |
| mine-opt | 107.20 | 93.40 | 83 | 15832 |
| gcc-O0 | 196.30 | 55.79 | 71 | 15992 |
| gcc-O2 | 342.82 | 24.28 | 368 | 15992 |
| clang-O2 | 656.93 | 35.48 | 74 | 16088 |
| dotnet-aot | 2203.23 | 57.88 | NA | 1465128 |

### hashloop — 10⁸ iteraciones = -5768770193658875776

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 104.40 | 327.58 | 68 | 15776 |
| mine-opt | 104.29 | 335.16 | 65 | 15776 |
| gcc-O0 | 201.23 | 108.73 | 84 | 16008 |
| gcc-O2 | 305.50 | 80.99 | 51 | 15968 |
| clang-O2 | 676.89 | 25.45 | 64 | 16064 |
| dotnet-aot | 2336.90 | 96.71 | NA | 1465136 |

### sieve — primos < 2·10⁶ = 148933

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 105.63 | 42.06 | 162 | 15824 |
| mine-opt | 105.07 | 43.04 | 159 | 15824 |
| gcc-O0 | 208.92 | 36.89 | 88 | 16064 |
| gcc-O2 | 314.23 | 26.62 | 60 | 16064 |
| clang-O2 | 728.91 | 32.60 | 80 | 16160 |
| dotnet-aot | 2222.18 | 41.71 | NA | 1465128 |

### matmul — `c[199,199]` = 333300

| Config | Compile (ms) | Run (ms) | Asm (líneas) | Binario (bytes) |
|---|---|---|---|---|
| mine-noopt | 106.20 | 38.14 | 364 | 15824 |
| mine-opt | 108.08 | 38.48 | 361 | 15824 |
| gcc-O0 | 215.71 | 29.86 | 210 | 16064 |
| gcc-O2 | 347.62 | 18.88 | 142 | 16112 |
| clang-O2 | 740.43 | 17.42 | 151 | 16208 |
| dotnet-aot | 2205.41 | 41.76 | NA | 1465128 |

*(Datos crudos reproducibles en `benchmarks/results/results.csv`; tabla
autogenerada en `benchmarks/results/results.md`.)*

## 4. Análisis

**Rendimiento de ejecución (lo esencial).** Tomando `mine-opt` como referencia:

| Benchmark | mine-opt vs gcc-O0 | mine-opt vs gcc-O2 | mine-opt vs clang-O2 | mine-opt vs dotnet-aot |
|---|---|---|---|---|
| fib | 1.7× más lento | 3.8× más lento | 2.6× más lento | 1.6× más lento |
| hashloop | 3.1× más lento | 4.1× más lento | 13.2× más lento | 3.5× más lento |
| sieve | 1.2× más lento | 1.6× más lento | 1.3× más lento | 1.0× más lento |
| matmul | 1.3× más lento | 2.0× más lento | 2.2× más lento | 0.9× más rápido |

El compilador genera código **correcto y funcional**, con un costo típico de
**1.6–4.1× frente a `gcc -O2`** y a menudo **cercano a `gcc -O0` o
`dotnet-aot`**. Las causas del
diferencial son las esperables de un generador de código simple:

- **Sin asignación de registros global.** El compilador usa un esquema de
  pila/acumulador: casi cada operación toca memoria. GCC/Clang mantienen las
  variables calientes (`i`, `h`, acumuladores) en registros. Esto explica el
  peor caso, `hashloop` (bucle apretado sobre una variable): Clang además aplica
  *strength reduction* y desenrollado y llega a ser **13.2×** más rápido.
- **Selección de instrucciones ingenua** y ausencia de LICM, desenrollado de
  bucles y vectorización.

**Efecto de `--opt` (optimización propia).** La diferencia `mine-opt` vs
`mine-noopt` en tiempo de ejecución es **marginal** (dentro del ruido: fib
93.40 vs 94.96 ms; sieve 43.04 vs 42.06 ms). El efecto visible está en el
**tamaño del ensamblador**, que el peephole reduce de forma consistente
(fib 89→83, hashloop 68→65, sieve 162→159, matmul 364→361 líneas). Interpretación
honesta: las optimizaciones implementadas (plegado de constantes, Sethi-Ullman,
peephole) son **locales**; no atacan el cuello de botella real —tráfico a
memoria por falta de asignación de registros— por lo que su impacto en tiempo es
pequeño en estos núcleos. Para exhibir mejoras de tiempo medibles con `--opt`
convendría un benchmark saturado de subexpresiones constantes (donde el plegado
sí dispara) o implementar asignación de registros.

**Tiempo de compilación.** Aquí este compilador aparece **más rápido**
(~103–108 ms) que `gcc -O0` (~196–216 ms), `gcc -O2` (~305–348 ms),
`clang -O2` (~657–741 ms) y `dotnet-aot` (~2.2–2.3 s). La lectura correcta es
matizada: (a) el lenguaje
aceptado es un subconjunto mínimo, sin el frontend ni el pipeline de
optimización de un compilador industrial; (b) esos ~103–108 ms están
**dominados por el paso `g++` de ensamblado y enlace**, no por el frontend
propio, que es casi instantáneo. Es decir, *compilar menos y optimizar menos*
cuesta menos tiempo, como es de esperar. Aun así muestra un punto real: un
pipeline simple tiene baja latencia de compilación.

**Tamaño de binario y ensamblador.** Los ejecutables de `mine`, GCC y Clang son
casi idénticos en tamaño (~15.8–16.2 KB; domina el runtime de C enlazado en la
sección relevante). Los binarios `dotnet-aot` son mucho mayores (~1.46 MB)
porque son ejecutables self-contained con runtime mínimo AOT incluido. En
**líneas de ensamblador**, `gcc -O0`/`clang -O2` suelen ser más compactos que
`mine` (p. ej. sieve 60–88 vs 159); la excepción es `gcc -O2` en `fib` (368
líneas) por *inlining* agresivo de la recursión, que cambia código por velocidad.

**Semántica de entero usada.** Este compilador implementa `int` como entero de
**64 bits**, siguiendo la implementación base de `Sem15_code`. Por eso las
versiones en C no usan `int` de C, sino tipos de 64 bits equivalentes. En
`hashloop`, donde hay overflow, la versión C usa aritmética `uint64_t` definida
módulo 2^64 y luego imprime esos bits como entero signed en complemento a dos;
así se evita depender del overflow signed indefinido de C. *(Si el objetivo
fuera conformidad estricta con C#, `int` debería ser de 32 bits, pero ese no es
el modelo de esta implementación.)*

## 5. Conclusiones

- El compilador produce código **correcto** para los cuatro núcleos: salida
  idéntica a GCC, Clang y .NET Native AOT en las seis configuraciones.
- El costo en tiempo de ejecución es de **1.6–4.1× frente a `gcc -O2`**,
  competitivo con `gcc -O0` y cercano a `dotnet-aot` en algunos casos; el peor
  caso son los bucles muy apretados, donde la falta de asignación de registros
  pesa más.
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
