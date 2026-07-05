// Error semantico: no se puede reasignar una constante local.
class Program {
    static int Main() {
        const int edad = 20;
        edad = 21;
        return 0;
    }
}
