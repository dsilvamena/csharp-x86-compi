#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

// NOTA: este compilador implementa 'int' como entero de 64 bits, a diferencia
// de C# / C (32 bits). Para que el computo sea equivalente y definido en C,
// usamos aritmetica modulo 2^64 con uint64_t y luego imprimimos esos bits como
// entero con signo en complemento a dos, igual que el assembly generado.
static void print_i64_bits(uint64_t x) {
    if (x <= (uint64_t)INT64_MAX) {
        printf("%" PRIu64 "\n", x);
    } else {
        printf("-%" PRIu64 "\n", (~x) + 1);
    }
}

int main(void) {
    uint64_t h = 0;
    uint64_t i = 0;
    while (i < 100000000ULL) {
        h = h * 31 + i;
        i = i + 1;
    }
    print_i64_bits(h);
    return 0;
}
