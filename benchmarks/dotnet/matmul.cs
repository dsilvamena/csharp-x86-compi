using System;

class Program {
    static int Main() {
        int n = 200;
        long[,] a = new long[200, 200];
        long[,] b = new long[200, 200];
        long[,] c = new long[200, 200];

        int i = 0;
        while (i < n) {
            int j = 0;
            while (j < n) {
                a[i, j] = (i + j) % 100;
                b[i, j] = (i * j + 1) % 100;
                c[i, j] = 0;
                j = j + 1;
            }
            i = i + 1;
        }

        i = 0;
        while (i < n) {
            int j = 0;
            while (j < n) {
                long sum = 0;
                int k = 0;
                while (k < n) {
                    sum = sum + a[i, k] * b[k, j];
                    k = k + 1;
                }
                c[i, j] = sum;
                j = j + 1;
            }
            i = i + 1;
        }

        Console.WriteLine(c[n - 1, n - 1]);
        return 0;
    }
}
