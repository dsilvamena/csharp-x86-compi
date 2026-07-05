#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

static int64_t Fib(int64_t n) {
    if (n < 2) {
        return n;
    }
    return Fib(n - 1) + Fib(n - 2);
}

int main(void) {
    int64_t r = Fib(35);
    printf("%" PRId64 "\n", r);
    return 0;
}
