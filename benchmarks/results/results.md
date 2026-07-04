# Resultados de benchmarks

Tiempos en milisegundos (mejor de 3 compilaciones / 5 ejecuciones).

## fib  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 9227465 | 111.97 | 94.70 | 89 | 15792 |
| mine-opt | 9227465 | 113.37 | 94.56 | 83 | 15792 |
| gcc-O0 | 9227465 | 196.53 | 55.32 | 70 | 15776 |
| gcc-O2 | 9227465 | 347.94 | 24.00 | 397 | 15776 |
| clang-O2 | 9227465 | 654.89 | 36.90 | 78 | 16088 |

## hashloop  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | -5768770193658875776 | 112.40 | 317.54 | 68 | 15728 |
| mine-opt | -5768770193658875776 | 112.04 | 303.26 | 65 | 15728 |
| gcc-O0 | -5768770193658875776 | 197.87 | 107.03 | 47 | 15760 |
| gcc-O2 | -5768770193658875776 | 287.43 | 81.02 | 38 | 15760 |
| clang-O2 | -5768770193658875776 | 657.07 | 26.00 | 51 | 16064 |

## sieve  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 148933 | 112.63 | 44.03 | 162 | 15784 |
| mine-opt | 148933 | 112.67 | 44.13 | 159 | 15784 |
| gcc-O0 | 148933 | 207.15 | 33.79 | 91 | 15856 |
| gcc-O2 | 148933 | 311.52 | 22.74 | 61 | 15856 |
| clang-O2 | 148933 | 707.65 | 22.79 | 82 | 16160 |

## matmul  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 333300 | 112.95 | 39.15 | 364 | 15784 |
| mine-opt | 333300 | 113.43 | 38.90 | 361 | 15784 |
| gcc-O0 | 333300 | 212.49 | 29.17 | 208 | 15856 |
| gcc-O2 | 333300 | 346.71 | 16.55 | 226 | 15904 |
| clang-O2 | 333300 | 763.68 | 16.69 | 222 | 16208 |

