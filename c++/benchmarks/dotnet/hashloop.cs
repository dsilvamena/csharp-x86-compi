using System;

class Program {
    static int Main() {
        long h = 0;
        long i = 0;
        unchecked {
            while (i < 100000000L) {
                h = h * 31 + i;
                i = i + 1;
            }
        }
        Console.WriteLine(h);
        return 0;
    }
}
