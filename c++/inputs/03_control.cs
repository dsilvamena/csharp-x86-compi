class Program {
    static int Main() {
        int s = 0;
        for (int i = 0; i < 5; i = i + 1) {
            s = s + i;
        }
        if (s == 10) {
            Console.WriteLine(s);
        } else {
            Console.WriteLine(0);
        }
        return 0;
    }
}
