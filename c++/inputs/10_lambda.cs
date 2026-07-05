class Program {
    static int Main() {
        var inc = (int x) => x + 1;
        var combine = (int a, int b) => a * b + a;
        var isBig = (int n) => n > 10;

        Console.WriteLine(inc(41));
        Console.WriteLine(combine(6, 7));

        if (isBig(12)) {
            Console.WriteLine(1);
        } else {
            Console.WriteLine(0);
        }

        return 0;
    }
}
