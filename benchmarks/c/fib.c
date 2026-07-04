#include <stdio.h>

static int Fib(int n) {
    if (n < 2) {
        return n;
    }
    return Fib(n - 1) + Fib(n - 2);
}

int main(void) {
    int r = Fib(35);
    printf("%d\n", r);
    return 0;
}
