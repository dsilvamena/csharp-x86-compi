// Benchmark: 1D array, branches, non-unit strides (memory + control flow).
// Criba de Eratostenes: cuenta los primos menores que N.
// El arreglo se inicializa explicitamente porque 'new int[]' usa malloc
// (memoria no puesta a cero).
class Program {
    static int Main() {
        int n = 2000000;
        int[] comp = new int[n];

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

        Console.WriteLine(count);
        return 0;
    }
}
