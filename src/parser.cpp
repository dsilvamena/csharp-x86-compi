#include "parser.h"
#include "errors.h"

Parser::Parser(Lexer &lexer)
    : lexer(lexer), current(Token::END), previous(Token::END) {
  current = lexer.nextToken();
}

bool Parser::check(Token::Type t) const { return current.type == t; }
bool Parser::isAtEnd() const { return current.type == Token::END; }

void Parser::advance() {
  previous = current;
  if (!lookahead.empty()) {
    current = lookahead.front();
    lookahead.pop_front();
  } else if (!isAtEnd()) {
    current = lexer.nextToken();
  }
}

const Token &Parser::peek(size_t n) {
  while (lookahead.size() < n) lookahead.push_back(lexer.nextToken());
  return lookahead[n - 1];
}

bool Parser::match(Token::Type t) {
  if (check(t)) {
    advance();
    return true;
  }
  return false;
}

void Parser::error(const std::string &what) {
  throw CompileError("sintactico", what + " (se encontro '" + current.text + "' / " +
                                       Token::typeName(current.type) + ")",
                     current.line, current.col);
}

Token Parser::expect(Token::Type t, const std::string &what) {
  if (check(t)) {
    advance();
    return previous;
  }
  error(what);
}

bool Parser::isTypeStart() const {
  return check(Token::INT) || check(Token::BOOL) || check(Token::CHAR) ||
         check(Token::STRING);
}

bool Parser::looksLikeVarDecl() {
  if (isTypeStart()) return true;
  if (check(Token::ID)) {
    if (structNames.count(current.text)) return true;
    if (peek().type == Token::ID) return true;
  }
  return false;
}

std::string Parser::parseType() {
  std::string base;
  if (match(Token::INT)) base = "int";
  else if (match(Token::BOOL)) base = "bool";
  else if (match(Token::CHAR)) base = "char";
  else if (match(Token::STRING)) base = "string";
  else if (match(Token::VOID)) base = "void";
  else if (check(Token::ID)) { base = current.text; advance(); }
  else error("se esperaba un tipo");
  if (check(Token::LBRACKET) && peek().type == Token::RBRACKET) {
    advance(); advance();
    base += "[]";
  } else if (check(Token::LBRACKET) && peek().type == Token::COMA) {
    advance(); advance();
    expect(Token::RBRACKET, "se esperaba ']' en el tipo de matriz");
    base += "[,]";
  }
  // Sufijo de puntero (unsafe): int*, Point*, ...
  while (match(Token::MUL)) base += "*";
  return base;
}

Program *Parser::parseProgram() {
  Program *p = new Program();
  while (!isAtEnd()) {
    if (check(Token::STRUCT)) p->structs.push_back(parseStructDecl());
    else if (check(Token::CLASS)) p->classes.push_back(parseClassDecl());
    else error("se esperaba 'class' o 'struct'");
  }
  if (p->classes.empty()) error("el programa no contiene ninguna clase");
  return p;
}

StructDecl *Parser::parseStructDecl() {
  expect(Token::STRUCT, "se esperaba 'struct'");
  Token name = expect(Token::ID, "se esperaba el nombre de la estructura");
  StructDecl *s = new StructDecl();
  s->name = name.text;
  structNames.insert(s->name);
  expect(Token::LBRACE, "se esperaba '{' tras el nombre de la estructura");
  while (!check(Token::RBRACE) && !isAtEnd()) {
    StructField f;
    f.type = parseType();
    f.name = expect(Token::ID, "se esperaba el nombre del campo").text;
    expect(Token::SEMICOL, "se esperaba ';' tras el campo");
    s->fields.push_back(f);
  }
  expect(Token::RBRACE, "se esperaba '}' al cerrar la estructura");
  return s;
}

ClassDecl *Parser::parseClassDecl() {
  expect(Token::CLASS, "se esperaba 'class'");
  Token name = expect(Token::ID, "se esperaba el nombre de la clase");
  ClassDecl *c = new ClassDecl();
  c->name = name.text;
  expect(Token::LBRACE, "se esperaba '{' tras el nombre de la clase");
  while (!check(Token::RBRACE) && !isAtEnd()) {
    c->methods.push_back(parseMethodDecl());
  }
  expect(Token::RBRACE, "se esperaba '}' al cerrar la clase");
  return c;
}

MethodDecl *Parser::parseMethodDecl() {
  MethodDecl *m = new MethodDecl();
  m->isStatic = match(Token::STATIC);
  m->retType = parseType();
  Token name = expect(Token::ID, "se esperaba el nombre del metodo");
  m->name = name.text;
  expect(Token::LPAREN, "se esperaba '(' en la declaracion del metodo");
  if (!check(Token::RPAREN)) {
    do {
      Param param;
      param.type = parseType();
      param.name = expect(Token::ID, "se esperaba el nombre del parametro").text;
      m->params.push_back(param);
    } while (match(Token::COMA));
  }
  expect(Token::RPAREN, "se esperaba ')' tras los parametros");
  m->body = parseBlock();
  return m;
}

Block *Parser::parseBlock() {
  Token lb = expect(Token::LBRACE, "se esperaba '{'");
  Block *b = new Block();
  b->line = lb.line;
  b->col = lb.col;
  while (!check(Token::RBRACE) && !isAtEnd()) {
    b->stms.push_back(parseStatement());
  }
  expect(Token::RBRACE, "se esperaba '}'");
  return b;
}

Stm *Parser::parseStatement() {
  if (check(Token::CONSOLE)) return parsePrint();
  if (check(Token::IF)) return parseIf();
  if (check(Token::WHILE)) return parseWhile();
  if (check(Token::FOR)) return parseFor();
  if (check(Token::DO)) return parseDoWhile();
  if (check(Token::RETURN)) return parseReturn();
  if (check(Token::BREAK)) return parseBreak();
  if (check(Token::UNSAFE)) {
    Token kw = expect(Token::UNSAFE, "se esperaba 'unsafe'");
    UnsafeStm *u = new UnsafeStm(parseBlock());
    u->line = kw.line; u->col = kw.col;
    return u;
  }
  if (check(Token::LBRACE)) return parseBlock();
  if (looksLikeVarDecl()) {
    Stm *s = parseVarDecNoSemi();
    expect(Token::SEMICOL, "se esperaba ';' tras la declaracion");
    return s;
  }
  Stm *s = parseSimpleStmNoSemi();
  expect(Token::SEMICOL, "se esperaba ';' al final de la sentencia");
  return s;
}

Stm *Parser::parseVarDecNoSemi() {
  int L = current.line, C = current.col;
  std::string type = parseType();
  Token name = expect(Token::ID, "se esperaba el nombre de la variable");
  Exp *init = nullptr;
  if (match(Token::ASSIGN)) init = parseExpr();
  VarDecStm *vd = new VarDecStm(type, name.text, init);
  vd->line = L;
  vd->col = C;
  return vd;
}

Stm *Parser::parseSimpleStmNoSemi() {
  int L = current.line, C = current.col;
  Exp *lhs = parseExpr();
  if (match(Token::ASSIGN)) {
    Exp *rhs = parseExpr();
    AssignStm *a = new AssignStm(lhs, rhs);
    a->line = L;
    a->col = C;
    return a;
  }
  ExprStm *e = new ExprStm(lhs);
  e->line = L;
  e->col = C;
  return e;
}

Stm *Parser::parseIf() {
  Token kw = expect(Token::IF, "se esperaba 'if'");
  expect(Token::LPAREN, "se esperaba '(' tras 'if'");
  Exp *cond = parseExpr();
  expect(Token::RPAREN, "se esperaba ')' tras la condicion");
  Block *thenB = parseBlock();
  Block *elseB = nullptr;
  if (match(Token::ELSE)) {
    if (check(Token::IF)) {
      elseB = new Block();
      elseB->stms.push_back(parseIf());
    } else {
      elseB = parseBlock();
    }
  }
  IfStm *s = new IfStm(cond, thenB, elseB);
  s->line = kw.line;
  s->col = kw.col;
  return s;
}

Stm *Parser::parseWhile() {
  Token kw = expect(Token::WHILE, "se esperaba 'while'");
  expect(Token::LPAREN, "se esperaba '(' tras 'while'");
  Exp *cond = parseExpr();
  expect(Token::RPAREN, "se esperaba ')' tras la condicion");
  Block *body = parseBlock();
  WhileStm *s = new WhileStm(cond, body);
  s->line = kw.line;
  s->col = kw.col;
  return s;
}

Stm *Parser::parseFor() {
  Token kw = expect(Token::FOR, "se esperaba 'for'");
  expect(Token::LPAREN, "se esperaba '(' tras 'for'");
  Stm *init = nullptr;
  if (!check(Token::SEMICOL)) {
    if (looksLikeVarDecl()) init = parseVarDecNoSemi();
    else init = parseSimpleStmNoSemi();
  }
  expect(Token::SEMICOL, "se esperaba ';' tras el init del for");
  Exp *cond = nullptr;
  if (!check(Token::SEMICOL)) cond = parseExpr();
  expect(Token::SEMICOL, "se esperaba ';' tras la condicion del for");
  Stm *update = nullptr;
  if (!check(Token::RPAREN)) update = parseSimpleStmNoSemi();
  expect(Token::RPAREN, "se esperaba ')' al cerrar el for");
  Block *body = parseBlock();
  ForStm *s = new ForStm(init, cond, update, body);
  s->line = kw.line;
  s->col = kw.col;
  return s;
}

Stm *Parser::parseDoWhile() {
  Token kw = expect(Token::DO, "se esperaba 'do'");
  Block *body = parseBlock();
  expect(Token::WHILE, "se esperaba 'while' tras el cuerpo del do");
  expect(Token::LPAREN, "se esperaba '(' tras 'while'");
  Exp *cond = parseExpr();
  expect(Token::RPAREN, "se esperaba ')' tras la condicion");
  expect(Token::SEMICOL, "se esperaba ';' tras do-while");
  DoWhileStm *s = new DoWhileStm(body, cond);
  s->line = kw.line;
  s->col = kw.col;
  return s;
}

Stm *Parser::parseReturn() {
  Token kw = expect(Token::RETURN, "se esperaba 'return'");
  Exp *e = nullptr;
  if (!check(Token::SEMICOL)) e = parseExpr();
  expect(Token::SEMICOL, "se esperaba ';' tras return");
  ReturnStm *s = new ReturnStm(e);
  s->line = kw.line;
  s->col = kw.col;
  return s;
}

Stm *Parser::parseBreak() {
  Token kw = expect(Token::BREAK, "se esperaba 'break'");
  expect(Token::SEMICOL, "se esperaba ';' tras break");
  BreakStm *s = new BreakStm();
  s->line = kw.line;
  s->col = kw.col;
  return s;
}

Stm *Parser::parsePrint() {
  Token kw = expect(Token::CONSOLE, "se esperaba 'Console'");
  expect(Token::DOT, "se esperaba '.' tras 'Console'");
  Token method = expect(Token::ID, "se esperaba 'WriteLine' tras 'Console.'");
  if (method.text != "WriteLine" && method.text != "Write")
    throw CompileError("sintactico", "solo se soporta Console.WriteLine", method.line, method.col);
  expect(Token::LPAREN, "se esperaba '(' tras Console.WriteLine");
  Exp *e = parseExpr();
  expect(Token::RPAREN, "se esperaba ')' tras el argumento de WriteLine");
  expect(Token::SEMICOL, "se esperaba ';' tras Console.WriteLine(...)");
  PrintStm *s = new PrintStm(e);
  s->line = kw.line;
  s->col = kw.col;
  return s;
}

Exp *Parser::parseExpr() { return parseLogicalOr(); }

Exp *Parser::parseLogicalOr() {
  Exp *e = parseLogicalAnd();
  while (check(Token::OR)) {
    int L = current.line, C = current.col;
    advance();
    Exp *r = parseLogicalAnd();
    BinaryExp *b = new BinaryExp(e, r, OR_OP);
    b->line = L; b->col = C;
    e = b;
  }
  return e;
}

Exp *Parser::parseLogicalAnd() {
  Exp *e = parseEquality();
  while (check(Token::AND)) {
    int L = current.line, C = current.col;
    advance();
    Exp *r = parseEquality();
    BinaryExp *b = new BinaryExp(e, r, AND_OP);
    b->line = L; b->col = C;
    e = b;
  }
  return e;
}

Exp *Parser::parseEquality() {
  Exp *e = parseRelational();
  while (check(Token::EQ) || check(Token::NE)) {
    BinaryOp op = check(Token::EQ) ? EQ_OP : NE_OP;
    int L = current.line, C = current.col;
    advance();
    Exp *r = parseRelational();
    BinaryExp *b = new BinaryExp(e, r, op);
    b->line = L; b->col = C;
    e = b;
  }
  return e;
}

Exp *Parser::parseRelational() {
  Exp *e = parseAdditive();
  while (check(Token::LT) || check(Token::GT) || check(Token::LE) || check(Token::GE)) {
    BinaryOp op = check(Token::LT) ? LT_OP : check(Token::GT) ? GT_OP
                  : check(Token::LE) ? LE_OP : GE_OP;
    int L = current.line, C = current.col;
    advance();
    Exp *r = parseAdditive();
    BinaryExp *b = new BinaryExp(e, r, op);
    b->line = L; b->col = C;
    e = b;
  }
  return e;
}

Exp *Parser::parseAdditive() {
  Exp *e = parseMultiplicative();
  while (check(Token::PLUS) || check(Token::MINUS)) {
    BinaryOp op = check(Token::PLUS) ? ADD_OP : SUB_OP;
    int L = current.line, C = current.col;
    advance();
    Exp *r = parseMultiplicative();
    BinaryExp *b = new BinaryExp(e, r, op);
    b->line = L; b->col = C;
    e = b;
  }
  return e;
}

Exp *Parser::parseMultiplicative() {
  Exp *e = parseUnary();
  while (check(Token::MUL) || check(Token::DIV) || check(Token::MOD)) {
    BinaryOp op = check(Token::MUL) ? MUL_OP : check(Token::DIV) ? DIV_OP : MOD_OP;
    int L = current.line, C = current.col;
    advance();
    Exp *r = parseUnary();
    BinaryExp *b = new BinaryExp(e, r, op);
    b->line = L; b->col = C;
    e = b;
  }
  return e;
}

Exp *Parser::parseUnary() {
  // Cast explícito: '(' Tipo ')' Unary.  Se distingue de un paréntesis de
  // agrupación porque tras '(' viene un token de tipo primitivo (lookahead).
  if (check(Token::LPAREN)) {
    Token::Type nt = peek().type;
    if (nt == Token::INT || nt == Token::CHAR || nt == Token::BOOL || nt == Token::STRING) {
      int L = current.line, C = current.col;
      advance();  // (
      std::string t = parseType();
      expect(Token::RPAREN, "se esperaba ')' tras el tipo del cast");
      Exp *operand = parseUnary();
      CastExp *c = new CastExp(t, operand);
      c->line = L; c->col = C;
      return c;
    }
  }
  if (check(Token::MINUS) || check(Token::NOT)) {
    UnaryOp op = check(Token::MINUS) ? NEG_OP : NOT_OP;
    int L = current.line, C = current.col;
    advance();
    Exp *operand = parseUnary();
    UnaryExp *u = new UnaryExp(op, operand);
    u->line = L; u->col = C;
    return u;
  }
  // &x : dirección de una variable (unsafe).
  if (check(Token::AMP)) {
    int L = current.line, C = current.col;
    advance();
    Token id = expect(Token::ID, "se esperaba el nombre de la variable tras '&'");
    AddressOfExp *a = new AddressOfExp(id.text);
    a->line = L; a->col = C;
    return a;
  }
  // *p : desreferencia (unsafe).  En posición unaria, no es multiplicación.
  if (check(Token::MUL)) {
    int L = current.line, C = current.col;
    advance();
    Exp *operand = parseUnary();
    DerefExp *de = new DerefExp(operand);
    de->line = L; de->col = C;
    return de;
  }
  return parsePrimary();
}

std::vector<Exp *> Parser::parseArgs() {
  std::vector<Exp *> args;
  if (!check(Token::RPAREN)) {
    do {
      args.push_back(parseExpr());
    } while (match(Token::COMA));
  }
  return args;
}

Exp *Parser::parsePrimary() {
  int L = current.line, C = current.col;
  if (check(Token::NEW)) {
    advance();
    std::string et = parseType();
    expect(Token::LBRACKET, "se esperaba '[' tras 'new " + et + "'");
    Exp *first = parseExpr();
    if (match(Token::COMA)) {
      Exp *second = parseExpr();
      expect(Token::RBRACKET, "se esperaba ']' al cerrar 'new'");
      NewMatrixExp *m = new NewMatrixExp(et, first, second);
      m->line = L; m->col = C;
      return m;
    }
    expect(Token::RBRACKET, "se esperaba ']' al cerrar 'new'");
    NewArrayExp *na = new NewArrayExp(et, first);
    na->line = L; na->col = C;
    return na;
  }
  if (check(Token::NUM)) {
    long v = std::stol(current.text);
    advance();
    NumberExp *e = new NumberExp(v);
    e->line = L; e->col = C;
    return e;
  }
  if (check(Token::CHARLIT)) {
    int v = std::stoi(current.text);
    advance();
    CharExp *e = new CharExp(v);
    e->line = L; e->col = C;
    return e;
  }
  if (check(Token::STRINGLIT)) {
    std::string v = current.text;
    advance();
    StringExp *e = new StringExp(v);
    e->line = L; e->col = C;
    return e;
  }
  if (check(Token::TRUE)) { advance(); BoolExp *e = new BoolExp(true); e->line=L; e->col=C; return e; }
  if (check(Token::FALSE)) { advance(); BoolExp *e = new BoolExp(false); e->line=L; e->col=C; return e; }
  if (check(Token::ID)) {
    std::string name = current.text;
    advance();
    if (match(Token::LPAREN)) {
      CallExp *call = new CallExp(name);
      call->args = parseArgs();
      expect(Token::RPAREN, "se esperaba ')' al cerrar la llamada");
      call->line = L; call->col = C;
      return call;
    }
    if (match(Token::DOT)) {
      Token field = expect(Token::ID, "se esperaba el nombre del campo tras '.'");
      FieldExp *fe = new FieldExp(name, field.text);
      fe->line = L; fe->col = C;
      return fe;
    }
    if (match(Token::LBRACKET)) {
      Exp *idx = parseExpr();
      if (match(Token::COMA)) {
        Exp *colE = parseExpr();
        expect(Token::RBRACKET, "se esperaba ']' tras los indices de matriz");
        MatrixAccessExp *ma = new MatrixAccessExp(name, idx, colE);
        ma->line = L; ma->col = C;
        return ma;
      }
      expect(Token::RBRACKET, "se esperaba ']' tras el indice");
      ArrayAccessExp *aa = new ArrayAccessExp(name, idx);
      aa->line = L; aa->col = C;
      return aa;
    }
    IdExp *e = new IdExp(name);
    e->line = L; e->col = C;
    return e;
  }
  if (match(Token::LPAREN)) {
    Exp *e = parseExpr();
    expect(Token::RPAREN, "se esperaba ')'");
    return e;
  }
  error("se esperaba una expresion");
}
