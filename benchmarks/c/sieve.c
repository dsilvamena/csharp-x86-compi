#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int n = 2000000;
    int *comp = (int *)malloc((size_t)n * sizeof(int));

    int i = 0;
    while (i < n) {
        comp[i] = 0;
        i = i + 1;
    }

    int count = 0;
    int p = 2;
    while (p < n) {
        if (comp[p] == 0) {
            count = count + 1;
            int j = p + p;
            while (j < n) {
                comp[j] = 1;
                j = j + p;
            }
        }
        p = p + 1;
    }

    printf("%d\n", count);
    free(comp);
    return 0;
}
