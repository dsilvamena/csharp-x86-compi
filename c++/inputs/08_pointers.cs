class Program {
    static int Main() {
        int x = 10;
        unsafe {
            int* p = &x;
            Console.WriteLine(*p);
            *p = 42;
            Console.WriteLine(x);

            int y = 5;
            int* q = &y;
            *q = *p + *q;
            Console.WriteLine(y);
        }
        Console.WriteLine(x);
        return 0;
    }
}
