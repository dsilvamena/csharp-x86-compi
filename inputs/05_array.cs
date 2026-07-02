class Program {
  static int Main() {
    int[] a = new int[3];

    a[0] = 4;
    a[1] = 5;
    a[2] = a[0] + a[1];

    Console.WriteLine(a[2]);

    return 0;
  }
}