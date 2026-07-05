#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

int main(void) {
    int64_t n = 2000000;
    int64_t *comp = (int64_t *)malloc((size_t)n * sizeof(int64_t));

    int64_t i = 0;
    while (i < n) {
        comp[i] = 0;
        i = i + 1;
    }

    int64_t count = 0;
    int64_t p = 2;
    while (p < n) {
        if (comp[p] == 0) {
            count = count + 1;
            int64_t j = p + p;
            while (j < n) {
                comp[j] = 1;
                j = j + p;
            }
        }
        p = p + 1;
    }

    printf("%" PRId64 "\n", count);
    free(comp);
    return 0;
}
