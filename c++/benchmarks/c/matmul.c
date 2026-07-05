#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

int main(void) {
    int64_t n = 200;
    int64_t *a = (int64_t *)malloc((size_t)n * (size_t)n * sizeof(int64_t));
    int64_t *b = (int64_t *)malloc((size_t)n * (size_t)n * sizeof(int64_t));
    int64_t *c = (int64_t *)malloc((size_t)n * (size_t)n * sizeof(int64_t));

    int64_t i = 0;
    while (i < n) {
        int64_t j = 0;
        while (j < n) {
            a[i * n + j] = (i + j) % 100;
            b[i * n + j] = (i * j + 1) % 100;
            c[i * n + j] = 0;
            j = j + 1;
        }
        i = i + 1;
    }

    i = 0;
    while (i < n) {
        int64_t j = 0;
        while (j < n) {
            int64_t sum = 0;
            int64_t k = 0;
            while (k < n) {
                sum = sum + a[i * n + k] * b[k * n + j];
                k = k + 1;
            }
            c[i * n + j] = sum;
            j = j + 1;
        }
        i = i + 1;
    }

    printf("%" PRId64 "\n", c[(n - 1) * n + (n - 1)]);
    free(a);
    free(b);
    free(c);
    return 0;
}
