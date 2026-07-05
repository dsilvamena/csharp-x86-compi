# Resultados de benchmarks

Tiempos en milisegundos (mejor de 3 compilaciones / 5 ejecuciones).

## fib  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 9227465 | 103.72 | 94.96 | 89 | 15832 |
| mine-opt | 9227465 | 107.20 | 93.40 | 83 | 15832 |
| gcc-O0 | 9227465 | 196.30 | 55.79 | 71 | 15992 |
| gcc-O2 | 9227465 | 342.82 | 24.28 | 368 | 15992 |
| clang-O2 | 9227465 | 656.93 | 35.48 | 74 | 16088 |
| dotnet-aot | 9227465 | 2203.23 | 57.88 | NA | 1465128 |

## hashloop  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | -5768770193658875776 | 104.40 | 327.58 | 68 | 15776 |
| mine-opt | -5768770193658875776 | 104.29 | 335.16 | 65 | 15776 |
| gcc-O0 | -5768770193658875776 | 201.23 | 108.73 | 84 | 16008 |
| gcc-O2 | -5768770193658875776 | 305.50 | 80.99 | 51 | 15968 |
| clang-O2 | -5768770193658875776 | 676.89 | 25.45 | 64 | 16064 |
| dotnet-aot | -5768770193658875776 | 2336.90 | 96.71 | NA | 1465136 |

## sieve  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 148933 | 105.63 | 42.06 | 162 | 15824 |
| mine-opt | 148933 | 105.07 | 43.04 | 159 | 15824 |
| gcc-O0 | 148933 | 208.92 | 36.89 | 88 | 16064 |
| gcc-O2 | 148933 | 314.23 | 26.62 | 60 | 16064 |
| clang-O2 | 148933 | 728.91 | 32.60 | 80 | 16160 |
| dotnet-aot | 148933 | 2222.18 | 41.71 | NA | 1465128 |

## matmul  (salidas OK (todas iguales))

| Config | Salida | Compile (ms) | Run (ms) | Asm (lineas) | Binario (bytes) |
|---|---|---|---|---|---|
| mine-noopt | 333300 | 106.20 | 38.14 | 364 | 15824 |
| mine-opt | 333300 | 108.08 | 38.48 | 361 | 15824 |
| gcc-O0 | 333300 | 215.71 | 29.86 | 210 | 16064 |
| gcc-O2 | 333300 | 347.62 | 18.88 | 142 | 16112 |
| clang-O2 | 333300 | 740.43 | 17.42 | 151 | 16208 |
| dotnet-aot | 333300 | 2205.41 | 41.76 | NA | 1465128 |

