# Resultados de benchmarks

Tiempos en milisegundos (mejor de 3 compilaciones / 5 ejecuciones).

## fib  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 9227465 | 111.48 | 94.10 | 89 | 15792 |
| mine-opt | 9227465 | 110.68 | 94.65 | 83 | 15792 |
| gcc-O0 | 9227465 | 196.95 | 55.02 | 70 | 15776 |
| gcc-O2 | 9227465 | 350.04 | 24.71 | 375 | 15776 |
| clang-O2 | 9227465 | 649.66 | 36.73 | 78 | 16088 |

## hashloop  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | -1206259584 | 112.33 | 427.74 | 68 | 15728 |
| mine-opt | -1206259584 | 117.03 | 420.43 | 65 | 15728 |
| gcc-O0 | -1206259584 | 199.05 | 129.46 | 47 | 15760 |
| gcc-O2 | -1206259584 | 291.37 | 103.49 | 38 | 15760 |
| clang-O2 | -1206259584 | 678.41 | 28.61 | 45 | 16064 |

## sieve  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 148933 | 113.96 | 45.09 | 162 | 15784 |
| mine-opt | 148933 | 115.50 | 43.86 | 159 | 15784 |
| gcc-O0 | 148933 | 217.94 | 34.27 | 91 | 15856 |
| gcc-O2 | 148933 | 314.97 | 23.04 | 67 | 15856 |
| clang-O2 | 148933 | 707.62 | 23.38 | 82 | 16160 |

## matmul  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 333300 | 114.22 | 39.12 | 381 | 15784 |
| mine-opt | 333300 | 114.17 | 39.52 | 378 | 15784 |
| gcc-O0 | 333300 | 218.70 | 28.79 | 207 | 15856 |
| gcc-O2 | 333300 | 374.12 | 16.42 | 226 | 15904 |
| clang-O2 | 333300 | 766.76 | 16.59 | 222 | 16208 |

