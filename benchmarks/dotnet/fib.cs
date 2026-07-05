using System;

class Program {
    static long Fib(long n) {
        if (n < 2) {
            return n;
        }
        return Fib(n - 1) + Fib(n - 2);
    }

    static int Main() {
        long r = Fib(35);
        Console.WriteLine(r);
        return 0;
    }
}
