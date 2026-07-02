// Etapa 14: punteros unsafe (int*, &x, *p) dentro de un bloque 'unsafe'.
class Program {
    static int Main() {
        int x = 10;
        unsafe {
            int* p = &x;
            Console.WriteLine(*p);   // 10
            *p = 42;                 // modifica x a través del puntero
            Console.WriteLine(x);    // 42

            int y = 5;
            int* q = &y;
            *q = *p + *q;            // 42 + 5 = 47
            Console.WriteLine(y);    // 47
        }
        Console.WriteLine(x);        // 42 (persiste fuera del bloque)
        return 0;
    }
}
