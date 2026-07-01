#ifndef AST_H
#define AST_H

#include <string>
#include <vector>

class Visitor;

enum BinaryOp {
  ADD_OP, SUB_OP, MUL_OP, DIV_OP, MOD_OP,
  LT_OP, GT_OP, LE_OP, GE_OP, EQ_OP, NE_OP,
  AND_OP, OR_OP
};
enum UnaryOp { NEG_OP, NOT_OP };

const char *binaryOpName(BinaryOp op);

class Exp {
public:
  bool isConstant = false;
  long constantValue = 0;
  int label = 0;
  int line = 0, col = 0;
  std::string inferredType;

  virtual int accept(Visitor *v) = 0;
  virtual ~Exp() = default;
};

class NumberExp : public Exp {
public:
  long value;
  explicit NumberExp(long v) : value(v) {}
  int accept(Visitor *v) override;
};

class BoolExp : public Exp {
public:
  bool value;
  explicit BoolExp(bool v) : value(v) {}
  int accept(Visitor *v) override;
};

class CharExp : public Exp {
public:
  int value;
  explicit CharExp(int v) : value(v) {}
  int accept(Visitor *v) override;
};

class StringExp : public Exp {
public:
  std::string value;
  int labelId = -1;
  explicit StringExp(std::string v) : value(std::move(v)) {}
  int accept(Visitor *v) override;
};

class IdExp : public Exp {
public:
  std::string name;
  explicit IdExp(std::string n) : name(std::move(n)) {}
  int accept(Visitor *v) override;
};

class BinaryExp : public Exp {
public:
  Exp *left;
  Exp *right;
  BinaryOp op;
  BinaryExp(Exp *l, Exp *r, BinaryOp o) : left(l), right(r), op(o) {}
  int accept(Visitor *v) override;
  ~BinaryExp() override { delete left; delete right; }
};

class UnaryExp : public Exp {
public:
  UnaryOp op;
  Exp *operand;
  UnaryExp(UnaryOp o, Exp *e) : op(o), operand(e) {}
  int accept(Visitor *v) override;
  ~UnaryExp() override { delete operand; }
};

class CallExp : public Exp {
public:
  std::string name;
  std::vector<Exp *> args;
  explicit CallExp(std::string n) : name(std::move(n)) {}
  int accept(Visitor *v) override;
  ~CallExp() override { for (auto a : args) delete a; }
};

class Param {
public:
  std::string type;
  std::string name;
};

class Stm {
public:
  int line = 0, col = 0;
  virtual int accept(Visitor *v) = 0;
  virtual ~Stm() = default;
};

class Block : public Stm {
public:
  std::vector<Stm *> stms;
  int accept(Visitor *v) override;
  ~Block() override { for (auto s : stms) delete s; }
};

class VarDecStm : public Stm {
public:
  std::string type;
  std::string name;
  Exp *init;
  VarDecStm(std::string t, std::string n, Exp *i)
      : type(std::move(t)), name(std::move(n)), init(i) {}
  int accept(Visitor *v) override;
  ~VarDecStm() override { delete init; }
};

class AssignStm : public Stm {
public:
  Exp *target;
  Exp *value;
  AssignStm(Exp *t, Exp *v) : target(t), value(v) {}
  int accept(Visitor *v) override;
  ~AssignStm() override { delete target; delete value; }
};

class ExprStm : public Stm {
public:
  Exp *e;
  explicit ExprStm(Exp *e) : e(e) {}
  int accept(Visitor *v) override;
  ~ExprStm() override { delete e; }
};

class PrintStm : public Stm {
public:
  Exp *e;
  PrintStm(Exp *e) : e(e) {}
  int accept(Visitor *v) override;
  ~PrintStm() override { delete e; }
};

class IfStm : public Stm {
public:
  Exp *cond;
  Block *thenB;
  Block *elseB;
  IfStm(Exp *c, Block *t, Block *e) : cond(c), thenB(t), elseB(e) {}
  int accept(Visitor *v) override;
  ~IfStm() override { delete cond; delete thenB; delete elseB; }
};

class WhileStm : public Stm {
public:
  Exp *cond;
  Block *body;
  WhileStm(Exp *c, Block *b) : cond(c), body(b) {}
  int accept(Visitor *v) override;
  ~WhileStm() override { delete cond; delete body; }
};

class DoWhileStm : public Stm {
public:
  Block *body;
  Exp *cond;
  DoWhileStm(Block *b, Exp *c) : body(b), cond(c) {}
  int accept(Visitor *v) override;
  ~DoWhileStm() override { delete body; delete cond; }
};

class ForStm : public Stm {
public:
  Stm *init;
  Exp *cond;
  Stm *update;
  Block *body;
  ForStm(Stm *i, Exp *c, Stm *u, Block *b) : init(i), cond(c), update(u), body(b) {}
  int accept(Visitor *v) override;
  ~ForStm() override { delete init; delete cond; delete update; delete body; }
};

class ReturnStm : public Stm {
public:
  Exp *e;
  explicit ReturnStm(Exp *e) : e(e) {}
  int accept(Visitor *v) override;
  ~ReturnStm() override { delete e; }
};

class BreakStm : public Stm {
public:
  int accept(Visitor *v) override;
};

class MethodDecl {
public:
  std::string retType;
  std::string name;
  std::vector<Param> params;
  Block *body;
  bool isStatic;
  MethodDecl() : body(nullptr), isStatic(false) {}
  int accept(Visitor *v);
  ~MethodDecl() { delete body; }
};

class ClassDecl {
public:
  std::string name;
  std::vector<MethodDecl *> methods;
  int accept(Visitor *v);
  ~ClassDecl() { for (auto m : methods) delete m; }
};

class Program {
public:
  std::vector<ClassDecl *> classes;
  int accept(Visitor *v);
  ~Program() { for (auto c : classes) delete c; }
};

std::string printAst(Program *p);

#endif
