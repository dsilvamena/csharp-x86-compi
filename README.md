# csharp-x86-compi

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
