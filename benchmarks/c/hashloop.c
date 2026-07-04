#include <stdio.h>

// Benchmark: tight arithmetic loop with a data dependency (multiply-accumulate).
// Usa int de 32 bits, igual que el int del compilador y el int usual de GCC/Clang
// en linux/amd64. GCC/Clang se compilan con -fwrapv para fijar overflow signed.
int main(void) {
    int h = 0;
    int i = 0;
    while (i < 100000000) {
        h = h * 31 + i;
        i = i + 1;
    }
    printf("%d\n", h);
    return 0;
}
