# csharp-x86-compi

Compilador de un subconjunto de C# a ensamblador x86-64 (AT&T / sintaxis GNU),
escrito en C++17. Flujo: `archivo .cs → Lexer → Parser descendente recursivo →
AST con accept(Visitor*) → TypeCheckerVisitor → GenCodeVisitor → x86-64 AT&T`.
Con `--opt` (por defecto) se aplican plegado de constantes, numeración de
Sethi-Ullman y un peephole textual sobre el `.s`.

## Requisitos

- `g++` con soporte C++17 (macOS o Linux).
- El `.s` generado es para **Linux ELF** (`printf@PLT`, `-no-pie`): en Linux se
  ensambla directo con `g++`; en macOS se necesita **Docker** para ejecutarlo.

## Compilar el compilador

```bash
make          # produce ./compiler
```

## Uso

```bash
./compiler inputs/07_cast.cs                 # -> outputs/07_cast.s   [--opt por defecto]
./compiler inputs/07_cast.cs --no-opt        # sin optimizaciones
./compiler inputs/07_cast.cs --tokens --ast  # depuración: tokens y AST
./compiler inputs/07_cast.cs -o salida.s     # ruta de salida explícita
```

La carpeta `outputs/` se crea automáticamente al compilar (no forma parte del
repositorio; `make clean` la elimina).

Para ensamblar y ejecutar el `.s` en **Linux**:

```bash
g++ -no-pie outputs/07_cast.s -o outputs/07_cast && ./outputs/07_cast
```

En **macOS** (Apple Silicon o Intel), vía Docker:

```bash
docker run --rm --platform linux/amd64 -v "$PWD/outputs:/w" gcc:13 \
  bash -c 'cd /w && g++ -no-pie 07_cast.s -o 07_cast && ./07_cast'
```

## Compilar / ejecutar todos los inputs

```bash
make compile-all   # compila todos los inputs/*.cs -> outputs/*.s (nativo)
make run-all       # además ensambla y ejecuta cada .s en Docker (gcc:13)
make clean         # borra el binario y outputs/
```

## Pruebas

La suite (`scripts/run_tests.sh`) recorre `inputs/*.cs`:

- Para cada programa válido lo compila con `--opt` y con `--no-opt`, ensambla con
  `g++ -no-pie`, lo ejecuta y compara su salida con `tests/expected/<name>.txt`.
  Exige además que `--opt` y `--no-opt` produzcan **la misma** salida.
- Para cada `inputs/err_*.cs` exige que la compilación **falle** con la fase
  esperada (léxico / sintáctico / semántico), deducida del nombre del archivo.

Las salidas de referencia viven en `tests/expected/` (versionadas) y **no** en
`outputs/`, para que sobrevivan a `make clean` (que borra `outputs/`).

En **Linux**:

```bash
make test                  # compila y corre la suite
```

En **macOS** (el `.s` es Linux ELF, no corre nativo), vía Docker (gcc:13):

```bash
scripts/docker.sh test     # reconstruye dentro del contenedor + suite completa
scripts/docker.sh gen      # regenera tests/expected/ (solo si el cambio es intencional)
scripts/docker.sh run inputs/07_cast.cs   # compila, linka y ejecuta un programa
```

Regenera `tests/expected/` con `gen` únicamente cuando un cambio de salida sea
intencional, y revisa el diff.

## Gramatica del compilador

```bnf
Program        ::= (StructDecl | ClassDecl)+

StructDecl     ::= "struct" id "{" FieldDecl* "}"

FieldDecl      ::= Type id ";"

ClassDecl      ::= "class" id "{" MethodDecl* "}"

MethodDecl     ::= ["static"] Type id "(" Params ")" Block

Params         ::= epsilon
                 | Param ("," Param)*

Param          ::= Type id

Type           ::= "int"
                 | "bool"
                 | "char"
                 | "string"
                 | "void"
                 | id
                 | Type "[]"
                 | Type "[,]"

VarType        ::= "int"
                 | "bool"
                 | "char"
                 | "string"
                 | id
                 | VarType "[]"
                 | VarType "[,]"

Block          ::= "{" Stm* "}"

Stm            ::= VarDec ";"
                 | SimpleStm ";"
                 | PrintStm
                 | IfStm
                 | WhileStm
                 | ForStm
                 | DoWhileStm
                 | ReturnStm
                 | BreakStm
                 | Block

VarDec         ::= VarType id ["=" Expr]

SimpleStm      ::= Expr ["=" Expr]

PrintStm       ::= "Console" "." ("WriteLine" | "Write") "(" Expr ")" ";"

IfStm          ::= "if" "(" Expr ")" Block ["else" (Block | IfStm)]

WhileStm       ::= "while" "(" Expr ")" Block

ForStm         ::= "for" "(" [ForInit] ";" [Expr] ";" [SimpleStm] ")" Block

ForInit        ::= VarDec
                 | SimpleStm

DoWhileStm     ::= "do" Block "while" "(" Expr ")" ";"

ReturnStm      ::= "return" [Expr] ";"

BreakStm       ::= "break" ";"

Expr           ::= LogicalOr

LogicalOr      ::= LogicalAnd ("||" LogicalAnd)*

LogicalAnd     ::= Equality ("&&" Equality)*

Equality       ::= Relational (("==" | "!=") Relational)*

Relational     ::= Additive (("<" | ">" | "<=" | ">=") Additive)*

Additive       ::= Multiplicative (("+" | "-") Multiplicative)*

Multiplicative ::= Unary (("*" | "/" | "%") Unary)*

Unary          ::= ("-" | "!") Unary
                 | "(" Type ")" Unary
                 | Primary

Primary        ::= num
                 | charlit
                 | stringlit
                 | "true"
                 | "false"
                 | "new" Type "[" Expr "]"
                 | "new" Type "[" Expr "," Expr "]"
                 | id
                 | id "(" Args ")"
                 | id "." id
                 | id "[" Expr "]"
                 | id "[" Expr "," Expr "]"
                 | "(" Expr ")"

Args           ::= epsilon
                 | Expr ("," Expr)*
```
