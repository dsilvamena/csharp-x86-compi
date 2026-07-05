struct Punto {
  int x;
  int y;
}

class Program {
  static int Area(Punto p) {
    return p.x * p.y;
  }

  static int Main() {
    Punto p;
    p.x = 6;
    p.y = 7;

    Console.WriteLine(p.x);
    Console.WriteLine(p.y);
    Console.WriteLine(Area(p));

    return 0;
  }
}