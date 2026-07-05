class Program {
  static int Main() {
    int[,] m = new int[2, 3];

    m[0, 0] = 1;
    m[1, 2] = 7;

    Console.WriteLine(m[0, 0]);
    Console.WriteLine(m[1, 2]);

    return 0;
  }
}