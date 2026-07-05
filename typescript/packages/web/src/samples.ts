// One-click examples for the editor. Written in the C# subset accepted by
// the compiler CLI: top-level `class Program { static T M(...) {...} }`, entry
// `static int Main()`, `Console.WriteLine(...)`, structs declared at top level.
// (No `float`, no top-level `int main()`, no `print`.)
export interface Sample {
  id: string;
  label: string;
  source: string;
}

export const SAMPLES: Sample[] = [
  {
    id: 'fib',
    label: 'Fibonacci (benchmark)',
    source: `class Program {
  static int Fib(int n) {
    int a = 0;
    int b = 1;
    int i = 0;
    while (i < n) {
      int t = a + b;
      a = b;
      b = t;
      i = i + 1;
    }
    return a;
  }

  static int Main() {
    Console.WriteLine(Fib(20));
    return 0;
  }
}
`,
  },
  {
    id: 'variables',
    label: 'Variables & print',
    source: `class Program {
  static int Main() {
    int x = 2 + 3 * 4;
    Console.WriteLine(x);
    return 0;
  }
}
`,
  },
  {
    id: 'functions',
    label: 'Functions',
    source: `class Program {
  static int Suma(int a, int b) {
    return a + b;
  }

  static int Main() {
    int x = 5;
    int y = 10;
    int z = Suma(x, y);
    Console.WriteLine(z);
    return 0;
  }
}
`,
  },
  {
    id: 'if_else',
    label: 'if / else',
    source: `class Program {
  static int Main() {
    int x = 7;
    if (x > 5) {
      Console.WriteLine(1);
    } else {
      Console.WriteLine(0);
    }
    return 0;
  }
}
`,
  },
  {
    id: 'while',
    label: 'while loop',
    source: `class Program {
  static int Main() {
    int x = 0;
    while (x < 5) {
      Console.WriteLine(x);
      x = x + 1;
    }
    return 0;
  }
}
`,
  },
  {
    id: 'arrays',
    label: 'Arrays',
    source: `class Program {
  static int Main() {
    int[] a = new int[3];
    a[0] = 10;
    a[1] = 20;
    a[2] = a[0] + a[1];
    Console.WriteLine(a[2]);
    return 0;
  }
}
`,
  },
  {
    id: 'struct',
    label: 'Struct',
    source: `struct Punto {
  int x;
  int y;
}

class Program {
  static int Main() {
    Punto p;
    p.x = 10;
    p.y = 20;
    Console.WriteLine(p.x);
    return 0;
  }
}
`,
  },
  {
    id: 'types_for',
    label: 'Tipos & for',
    source: `class Program {
  static int Main() {
    string msg = "hola";
    bool ok = true;
    char c = 'A';
    for (int i = 0; i < 3; i = i + 1) {
      Console.WriteLine(i);
    }
    Console.WriteLine(msg);
    Console.WriteLine(ok);
    Console.WriteLine(c);
    return 0;
  }
}
`,
  },
  {
    id: 'const_fold',
    label: 'Constant folding',
    source: `class Program {
  static int Main() {
    int x = 10 + 5 * 2;
    Console.WriteLine(x);
    return 0;
  }
}
`,
  },
  {
    id: 'lambda',
    label: 'Lambdas (5g)',
    source: `class Program {
  static int Main() {
    var inc = (int x) => x + 1;
    var add = (int a, int b) => a + b;
    Console.WriteLine(inc(41));
    Console.WriteLine(add(20, 22));
    return 0;
  }
}
`,
  },
  {
    id: 'lexical_error',
    label: 'Error léxico',
    source: `class Program {
  static int Main() {
    int x = 5 @ 3;
    Console.WriteLine(x);
    return 0;
  }
}
`,
  },
  {
    id: 'syntax_error',
    label: 'Error sintáctico',
    source: `class Program {
  static int Main() {
    int x = 5
    Console.WriteLine(x);
    return 0;
  }
}
`,
  },
  {
    id: 'semantic_error',
    label: 'Error semántico',
    source: `class Program {
  static int Main() {
    x = 5;
    Console.WriteLine(x);
    return 0;
  }
}
`,
  },
];

export const DEFAULT_SAMPLE = SAMPLES[0];
