using System;

class Program {
    static int Main() {
        int n = 2000000;
        long[] comp = new long[n];

        int i = 0;
        while (i < n) {
            comp[i] = 0;
            i = i + 1;
        }

        long count = 0;
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
