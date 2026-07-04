// Benchmark: recursion / stack frames / call overhead.
// Calcula Fibonacci de forma recursiva ingenua (exponencial).
class Program {
    static int Fib(int n) {
        if (n < 2) {
            return n;
        }
        return Fib(n - 1) + Fib(n - 2);
    }

    static int Main() {
        int r = Fib(35);
        Console.WriteLine(r);
        return 0;
    }
}
