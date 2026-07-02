// Cast explícito (int)expr y (char)expr.
// La promoción char -> int ya es implícita; el cast (char)int trunca al byte.
class Program {
    static int Main() {
        char c = 'A';            // 65
        int n = c + 1;           // promoción char->int: 66
        Console.WriteLine(n);

        char d = (char)n;        // 66 -> 'B'
        Console.WriteLine(d);

        int trunc = (int)((char)321);  // 321 & 0xFF = 65 -> imprime como int
        Console.WriteLine(trunc);

        int suma = (int)'a' + (int)'z';  // 97 + 122 = 219
        Console.WriteLine(suma);
        return 0;
    }
}
