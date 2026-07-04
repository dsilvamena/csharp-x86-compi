#include <stdio.h>

// NOTA: este compilador implementa 'int' como entero de 64 bits, a diferencia
// de C# / C (32 bits). Para que el computo y la salida sean equivalentes, la
// version en C usa 'long long' (64 bits). Es el unico benchmark donde la
// anchura importa: los demas mantienen sus valores dentro de 32 bits.
int main(void) {
    long long h = 0;
    long long i = 0;
    while (i < 100000000) {
        h = h * 31 + i;
        i = i + 1;
    }
    printf("%lld\n", h);
    return 0;
}
