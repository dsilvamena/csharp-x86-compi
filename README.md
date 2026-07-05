# C# → x86-64 Compiler (C++)

A compiler for an academic, limited subset of **C#** that targets **x86-64 assembly
(AT&T / GNU syntax)**, written in **C++**, following the **Lab10 flow**:

```
archivo .cs → Lexer manual → Parser descendente recursivo → AST con accept(Visitor*)
            → liftLambdas (5g) → TypeCheckerVisitor
            → optimización (folding + Sethi-Ullman) → GenCodeVisitor
            → x86-64 AT&T (.s) → peephole → g++ -no-pie → ejecución del binario
```

> **The authoritative documentation for the compiler is [`c++/README.md`](c++/README.md)**
> (grammar, AST/Visitor design, ABI/stack frame, optimizations, tests, benchmark).

## Contents

```
c++/src/     The compiler (Lab10-style). Build with `make` (g++, C++17).
c++/legacy/  Previous IR → NASM/Intel → JSON pipeline, archived (not built).
backend/     Node bridge (re-wired): POST /api/compile spawns `compiler --json` and
typescript/  forwards the JSON; POST /api/run assembles + runs it in Docker. The React
             studio renders tokens/AST/x86/simulación/optimización/errores.
```

> Note: this repo previously had an IR→NASM/Intel pipeline driving a React/Node web app.
> The C++ compiler was rewritten to the Lab10
> AST→Visitor→AT&T flow; that old pipeline lives in `c++/legacy/` (not built). The web
> stack was **re-wired** to the new CLI via a `--json` mode — see below.

## Requirements

- **g++** (C++17) to build the compiler. The emitted `.s` is **AT&T for Linux ELF**
  (`printf@PLT`, `-no-pie`), so it assembles natively on Linux/x86-64 but **not** on
  macOS/Apple Silicon.
- **Node.js 18+** and **npm** for the backend bridge and the web studio.
- **Docker** (with the `gcc:13` image) only for **native Linux execution** — the
  x86 tab's *Ejecutar (Docker)* button and `POST /api/run` assemble + run inside a
  Linux container. Viewing tokens/AST/assembly/simulation/optimization/errors
  needs no Docker. (No LLVM/nasm/clang involved.)

## Build & run

```bash
# 1) Build the C++ compiler (host binary)
cd c++
make                      # produces ./compiler
scripts/docker.sh test    # optional: full suite in Docker (gcc:13) — 25/25

# 2) Start the backend bridge (port 8787)
cd ../backend
npm install
npm start

# 3) Start the web app (port 5173) — in another terminal, from the repo root
cd typescript
npm install
npm run dev               # runs @cstudio/web; Vite proxies /api → :8787
```

Then open <http://localhost:5173>. The web app sends your C# code to the backend,
which runs `compiler --json` and returns **tokens, AST, x86-64 AT&T assembly
(`--no-opt` + `--opt`), a C#↔x86 source map, the optimization report, and errors**.
The studio has two execution paths:

- **Run / Simulación**: runs the generated AT&T assembly in the browser with a
  JavaScript x86-64 simulator, showing stdout plus CPU/register/stack state.
- **Ejecutar (Docker)** in the x86 tab: assembles + runs the `.s` in Docker
  (`gcc:13`) and shows the real native stdout in **Salida**.

There are no IR/Symbols tabs: the current Lab10-style CLI emits neither.

Test the web with **Playwright** (required): `cd typescript/packages/web &&
npx playwright test`. Test the simulator directly with `npm run test:sim -w
@cstudio/web`. Build static files: `npm run build -w @cstudio/web` (output in
`typescript/packages/web/dist`).

> On macOS, after `scripts/docker.sh test` rebuilds `./compiler` for Linux, run
> `cd c++ && make clean && make` again before starting the backend (it needs the host binary).

## Using the compiler directly (CLI)

```bash
cd c++
./compiler path/to/program.cs              # -> outputs/<name>.s (AT&T)   [--opt default]
./compiler path/to/program.cs --no-opt --tokens --ast -o out.s
./compiler --json < path/to/program.cs     # the JSON contract used by the backend
g++ -no-pie outputs/<name>.s -o prog && ./prog   # assemble + run (Linux/Docker)
```

Flags: `--opt` (default) / `--no-opt`, `--tokens`, `--ast`, `--json`, `-o <out.s>`.
`--json` reads the source from **stdin** and emits one JSON document
(`{ success, tokens, ast, asm, asmOpt, asmSourceMap, optReport, errors, metrics }`)
for the web app.

## Notes / limitations

- This implements a **limited subset of C#** for an academic compilers course — not
  full C#. Limitations are documented honestly; no claim is made that it beats GCC/Clang.
- Web execution has two modes: fast **Simulación** in-browser through the JS x86-64
  simulator, and real native execution through backend `/api/run` inside Docker
  (`gcc:13`). Use Docker output as the fidelity check for the Linux target.
