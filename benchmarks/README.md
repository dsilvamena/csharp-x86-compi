# Benchmarks

Evaluación comparativa de `csharp-x86-compi` contra **GCC** (`-O0 -fwrapv`,
`-O2 -fwrapv`) y **Clang/LLVM** (`-O2 -fwrapv`), para el criterio
*Comparación Comercial* del proyecto.
Las versiones C se compilan con `-fwrapv` para fijar el mismo desbordamiento
signed de `int32` que emite este compilador.

Cada benchmark está escrito dos veces con **algoritmo y salida idénticos**:

```text
benchmarks/
  cs/        versiones en el subconjunto de C# de este compilador (.cs)
  c/         versiones equivalentes en C (.c)
  bench.sh          lanza la suite en Docker linux/amd64 (usar en macOS)
  bench_inner.sh    corre dentro del contenedor (no invocar directo)
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
`gcc -O2 -fwrapv`/`clang -O2 -fwrapv` no puedan eliminar el bucle.

## Cómo correr

Desde la raíz del repositorio:

```bash
benchmarks/bench.sh
```

Requiere Docker (en macOS/Apple Silicon corre `linux/amd64` emulado). El script:

1. Reconstruye el compilador dentro del contenedor (`.docker-build/compiler`).
2. Instala `clang`.
3. Para cada benchmark y configuración mide compilación, ejecución, líneas de
   asm y tamaño del binario, y **verifica que todas las salidas coincidan**.
4. Escribe `results/results.csv` y `results/results.md`.

En Linux nativo x86-64 puedes correr `bench_inner.sh` directamente:

```bash
make BUILD_DIR=.docker-build
COMPILER=./.docker-build/compiler bash benchmarks/bench_inner.sh
```

## Métrica y validez

- Se reporta el **mejor** tiempo (de 3 compilaciones / 5 ejecuciones) para
  reducir ruido.
- La comparación solo es válida si **todas las configuraciones producen la misma
  salida**; el reporte lo marca por benchmark.
- Bajo emulación (Apple Silicon) las comparaciones **relativas** son válidas;
  para tiempos absolutos, repetir en x86-64 Linux nativo.

Análisis completo y discusión de resultados en [`REPORT.md`](REPORT.md).
