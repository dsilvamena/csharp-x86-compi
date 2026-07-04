#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int n = 200;
    int *a = (int *)malloc((size_t)n * n * sizeof(int));
    int *b = (int *)malloc((size_t)n * n * sizeof(int));
    int *c = (int *)malloc((size_t)n * n * sizeof(int));

    int i = 0;
    while (i < n) {
        int j = 0;
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
        int j = 0;
        while (j < n) {
            int sum = 0;
            int k = 0;
            while (k < n) {
                sum = sum + a[i * n + k] * b[k * n + j];
                k = k + 1;
            }
            c[i * n + j] = sum;
            j = j + 1;
        }
        i = i + 1;
    }

    printf("%d\n", c[(n - 1) * n + (n - 1)]);
    free(a);
    free(b);
    free(c);
    return 0;
}
