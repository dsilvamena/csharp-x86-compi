// Benchmark: tight arithmetic loop with a data dependency (multiply-accumulate).
// La dependencia h = h*31 + i impide que el compilador cierre el bucle en
// forma analitica, forzando ~10^8 iteraciones reales. En este compilador,
// int se implementa como entero de 64 bits.
class Program {
    static int Main() {
        int h = 0;
        int i = 0;
        while (i < 100000000) {
            h = h * 31 + i;
            i = i + 1;
        }
        Console.WriteLine(h);
        return 0;
    }
}
