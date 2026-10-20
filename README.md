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
backend/    Local Node bridge and shared compilation handler.
api/        Vercel function: POST /api/compile spawns `compiler --json`.
typescript/ React studio: tokens/AST/x86/simulation/optimization/errors.
```

> Note: this repo previously had an IR→NASM/Intel pipeline driving a React/Node web app.
> The C++ compiler was rewritten to the Lab10
> AST→Visitor→AT&T flow; that old pipeline lives in `c++/legacy/` (not built). The web
> stack was **re-wired** to the new CLI via a `--json` mode — see below.

## Requirements

- **g++** (C++17) to build the compiler. The emitted `.s` is **AT&T for Linux ELF**
  (`printf@PLT`, `-no-pie`), so it assembles natively on Linux/x86-64 but **not** on
  macOS/Apple Silicon.
- **Node.js 24** and **npm** for the backend bridge and the web studio.
- **Docker** is optional for the compiler CLI's Linux regression tests on macOS.
  The web app uses only the browser simulator and requires no Docker.

## Deploy to Vercel

See [the deployment guide](docs/vercel.md) for project settings, preview checks
and production deployment. Import the repository root, use Node.js 24, and keep
`vercel.json` settings. Vercel builds the C++ compiler for Linux and serves
`/api/compile` as a Node.js function; simulation runs in the browser.

## Build & run

```bash
# 1) Build the C++ compiler (host binary)
cd c++
make                      # produces ./build/compiler
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
**Run / Simulación** opens the browser's JavaScript x86-64 debugger, initially
paused. Press **Ejecutar** to run to the end and inspect stdout, registers and stack.

There are no IR/Symbols tabs: the current Lab10-style CLI emits neither.

From the repository root, run `npm run test:e2e` for Playwright,
`npm run test:api` for the compilation API and fixture output comparisons,
and `npm run test:sim` for simulator unit tests. Build with `npm run build`
(output in `typescript/packages/web/dist`).

## Using the compiler directly (CLI)

```bash
cd c++
./build/compiler path/to/program.cs              # -> outputs/<name>.s (AT&T)   [--opt default]
./build/compiler path/to/program.cs --no-opt --tokens --ast -o out.s
./build/compiler --json < path/to/program.cs     # the JSON contract used by the backend
g++ -no-pie outputs/<name>.s -o prog && ./prog   # assemble + run (Linux/Docker)
```

Flags: `--opt` (default) / `--no-opt`, `--tokens`, `--ast`, `--json`, `-o <out.s>`.
`--json` reads the source from **stdin** and emits one JSON document
(`{ success, tokens, ast, asm, asmOpt, asmSourceMap, optReport, errors, metrics }`)
for the web app.

## Notes / limitations

- This implements a **limited subset of C#** for an academic compilers course — not
  full C#. Limitations are documented honestly; no claim is made that it beats GCC/Clang.
- Web execution uses the in-browser JavaScript x86-64 simulator. Native assembly
  execution remains available through the compiler CLI on Linux or its Docker test scripts.
