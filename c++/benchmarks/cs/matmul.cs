// Benchmark: 2D arrays, nested loops, integer multiply (cache + arithmetic).
// Multiplica dos matrices NxN llenadas de forma deterministica e imprime
// el elemento inferior derecho del resultado como checksum.
// Los valores se mantienen acotados para que el ancho del entero no cambie
// la salida esperada.
class Program {
    static int Main() {
        int n = 200;
        int[,] a = new int[200, 200];
        int[,] b = new int[200, 200];
        int[,] c = new int[200, 200];

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
                int sum = 0;
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
