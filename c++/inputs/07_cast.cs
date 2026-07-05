class Program {
    static int Main() {
        char c = 'A';
        int n = c + 1;
        Console.WriteLine(n);

        char d = (char)n;
        Console.WriteLine(d);

        int trunc = (int)((char)321);
        Console.WriteLine(trunc);

        int suma = (int)'a' + (int)'z';
        Console.WriteLine(suma);
        return 0;
    }
}
