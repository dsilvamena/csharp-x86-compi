# Benchmarks

Evaluación comparativa de `csharp-x86-compi` contra **GCC** (`-O0`, `-O2`),
**Clang/LLVM** (`-O2`) y **.NET Native AOT**, para el criterio
*Comparación Comercial* del proyecto.

Cada benchmark está escrito en tres versiones con **algoritmo, salida y ancho de
entero equivalentes**. Como este compilador implementa `int` con 64 bits, las
versiones C usan tipos de 64 bits (`int64_t`/`uint64_t`) y las versiones .NET
usan `long` donde corresponde:

```text
benchmarks/
  cs/        versiones en el subconjunto de C# de este compilador (.cs)
  c/         versiones equivalentes en C (.c)
  dotnet/    versiones equivalentes para .NET Native AOT (.cs)
  bench.sh          lanza la suite en Docker linux/amd64 (usar en macOS)
  bench_wsl.sh      corre la suite directamente en WSL/Linux x86-64
  bench_inner.sh    implementación común usada por Docker y WSL
  results/          resultados generados (results.csv, results.md)
  REPORT.md         análisis técnico redactado (para el reporte del proyecto)
```

## Benchmarks incluidos

| Nombre | Qué mide | Parámetro |
|---|---|---|
| `fib` | recursión / overhead de llamada | `Fib(35)` |
| `hashloop` | bucle aritmético apretado (`h = h*31 + i`) | 10⁸ iteraciones |
| `sieve` | arreglo 1D + saltos (criba de Eratóstenes) | primos < 2·10⁶ |
| `matmul` | arreglos 2D + bucles anidados | matrices 200×200 |

Todos imprimen un checksum que depende de todo el cómputo, para que
`gcc -O2`/`clang -O2` no puedan eliminar el bucle.

## Cómo correr

### Opción 1: Docker

Desde la raíz del repositorio:

```bash
benchmarks/bench.sh
```

Requiere Docker (en macOS/Apple Silicon corre `linux/amd64` emulado). El script:

1. Reconstruye el compilador dentro del contenedor (`.docker-build/compiler`).
2. Usa la imagen `mcr.microsoft.com/dotnet/sdk:8.0` e instala toolchain nativo
   (`build-essential`, `clang`, `zlib1g-dev`).
3. Para cada benchmark y configuración mide compilación, ejecución, líneas de
   asm y tamaño del binario, y **verifica que todas las salidas coincidan**.
4. Escribe `results/results.csv` y `results/results.md`.

### Opción 2: WSL/Linux sin Docker

En WSL o Linux x86-64 nativo puedes correr la suite sin contenedores:

```bash
benchmarks/bench_wsl.sh
```

Dependencias base:

```bash
sudo apt update
sudo apt install -y build-essential clang
```

Para incluir `dotnet-aot`, instala además el SDK de .NET 8. Si `clang` o
`dotnet` no están disponibles, la suite omite `clang-O2` o `dotnet-aot` y lo
indica al final.

Para correr un subconjunto:

```bash
BENCHES="fib hashloop" benchmarks/bench.sh
BENCHES="fib hashloop" benchmarks/bench_wsl.sh
```

## Métrica y validez

- Se reporta el **mejor** tiempo (de 3 compilaciones / 5 ejecuciones) para
  reducir ruido.
- La comparación solo es válida si **todas las configuraciones producen la misma
  salida**; el reporte lo marca por benchmark.
- Bajo emulación (Apple Silicon) las comparaciones **relativas** son válidas;
  para tiempos absolutos, repetir en x86-64 Linux nativo.

Análisis completo y discusión de resultados en [`REPORT.md`](REPORT.md).
