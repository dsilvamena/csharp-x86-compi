#ifndef VISITOR_H
#define VISITOR_H

#include "ast.h"
#include "environment.h"
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class NumberExp; class BoolExp; class CharExp; class StringExp; class IdExp;
class BinaryExp; class UnaryExp; class CallExp; class FieldExp; class CastExp;
class AddressOfExp; class DerefExp;
class NewArrayExp; class ArrayAccessExp; class NewMatrixExp; class MatrixAccessExp;
class Block; class VarDecStm; class AssignStm; class ExprStm; class PrintStm;
class IfStm; class WhileStm; class DoWhileStm; class ForStm; class ReturnStm;
class BreakStm; class UnsafeStm; class StructDecl; class MethodDecl; class ClassDecl; class Program;

class Visitor {
public:
  virtual ~Visitor() = default;
  virtual int visit(NumberExp *e) = 0;
  virtual int visit(BoolExp *e) = 0;
  virtual int visit(CharExp *e) = 0;
  virtual int visit(StringExp *e) = 0;
  virtual int visit(IdExp *e) = 0;
  virtual int visit(BinaryExp *e) = 0;
  virtual int visit(UnaryExp *e) = 0;
  virtual int visit(CallExp *e) = 0;
  virtual int visit(FieldExp *e) = 0;
  virtual int visit(CastExp *e) = 0;
  virtual int visit(AddressOfExp *e) = 0;
  virtual int visit(DerefExp *e) = 0;
  virtual int visit(NewArrayExp *e) = 0;
  virtual int visit(ArrayAccessExp *e) = 0;
  virtual int visit(NewMatrixExp *e) = 0;
  virtual int visit(MatrixAccessExp *e) = 0;
  virtual int visit(Block *s) = 0;
  virtual int visit(VarDecStm *s) = 0;
  virtual int visit(AssignStm *s) = 0;
  virtual int visit(ExprStm *s) = 0;
  virtual int visit(PrintStm *s) = 0;
  virtual int visit(IfStm *s) = 0;
  virtual int visit(WhileStm *s) = 0;
  virtual int visit(DoWhileStm *s) = 0;
  virtual int visit(ForStm *s) = 0;
  virtual int visit(ReturnStm *s) = 0;
  virtual int visit(BreakStm *s) = 0;
  virtual int visit(UnsafeStm *s) = 0;
  virtual int visit(StructDecl *d) = 0;
  virtual int visit(MethodDecl *d) = 0;
  virtual int visit(ClassDecl *d) = 0;
  virtual int visit(Program *p) = 0;
};

class TypeCheckerVisitor : public Visitor {
  Environment env;
  std::unordered_map<std::string, MethodDecl *> methods;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> structs;
  std::string currentReturnType;
  int loopDepth = 0;
  int unsafeDepth = 0;   // >0 dentro de un bloque unsafe (habilita &x, *p)
  std::string lastType;

  std::string typeOf(Exp *e);
  bool isType(const std::string &t) const;

public:
  void check(Program *p);

  int visit(NumberExp *e) override;
  int visit(BoolExp *e) override;
  int visit(CharExp *e) override;
  int visit(StringExp *e) override;
  int visit(IdExp *e) override;
  int visit(BinaryExp *e) override;
  int visit(UnaryExp *e) override;
  int visit(CallExp *e) override;
  int visit(FieldExp *e) override;
  int visit(CastExp *e) override;
  int visit(AddressOfExp *e) override;
  int visit(DerefExp *e) override;
  int visit(NewArrayExp *e) override;
  int visit(ArrayAccessExp *e) override;
  int visit(NewMatrixExp *e) override;
  int visit(MatrixAccessExp *e) override;
  int visit(Block *s) override;
  int visit(VarDecStm *s) override;
  int visit(AssignStm *s) override;
  int visit(ExprStm *s) override;
  int visit(PrintStm *s) override;
  int visit(IfStm *s) override;
  int visit(WhileStm *s) override;
  int visit(DoWhileStm *s) override;
  int visit(ForStm *s) override;
  int visit(ReturnStm *s) override;
  int visit(BreakStm *s) override;
  int visit(UnsafeStm *s) override;
  int visit(StructDecl *d) override;
  int visit(MethodDecl *d) override;
  int visit(ClassDecl *d) override;
  int visit(Program *p) override;
};

class ConstantVisitor : public Visitor {
public:
  int folded = 0;
  void optimize(Program *p) { p->accept(this); }

  int visit(NumberExp *e) override;
  int visit(BoolExp *e) override;
  int visit(CharExp *e) override;
  int visit(StringExp *e) override;
  int visit(IdExp *e) override;
  int visit(BinaryExp *e) override;
  int visit(UnaryExp *e) override;
  int visit(CallExp *e) override;
  int visit(FieldExp *e) override;
  int visit(CastExp *e) override;
  int visit(AddressOfExp *e) override;
  int visit(DerefExp *e) override;
  int visit(NewArrayExp *e) override;
  int visit(ArrayAccessExp *e) override;
  int visit(NewMatrixExp *e) override;
  int visit(MatrixAccessExp *e) override;
  int visit(Block *s) override;
  int visit(VarDecStm *s) override;
  int visit(AssignStm *s) override;
  int visit(ExprStm *s) override;
  int visit(PrintStm *s) override;
  int visit(IfStm *s) override;
  int visit(WhileStm *s) override;
  int visit(DoWhileStm *s) override;
  int visit(ForStm *s) override;
  int visit(ReturnStm *s) override;
  int visit(BreakStm *s) override;
  int visit(UnsafeStm *s) override;
  int visit(StructDecl *d) override;
  int visit(MethodDecl *d) override;
  int visit(ClassDecl *d) override;
  int visit(Program *p) override;
};

class SethiUllmanVisitor : public Visitor {
public:
  void compute(Program *p) { p->accept(this); }

  int visit(NumberExp *e) override;
  int visit(BoolExp *e) override;
  int visit(CharExp *e) override;
  int visit(StringExp *e) override;
  int visit(IdExp *e) override;
  int visit(BinaryExp *e) override;
  int visit(UnaryExp *e) override;
  int visit(CallExp *e) override;
  int visit(FieldExp *e) override;
  int visit(CastExp *e) override;
  int visit(AddressOfExp *e) override;
  int visit(DerefExp *e) override;
  int visit(NewArrayExp *e) override;
  int visit(ArrayAccessExp *e) override;
  int visit(NewMatrixExp *e) override;
  int visit(MatrixAccessExp *e) override;
  int visit(Block *s) override;
  int visit(VarDecStm *s) override;
  int visit(AssignStm *s) override;
  int visit(ExprStm *s) override;
  int visit(PrintStm *s) override;
  int visit(IfStm *s) override;
  int visit(WhileStm *s) override;
  int visit(DoWhileStm *s) override;
  int visit(ForStm *s) override;
  int visit(ReturnStm *s) override;
  int visit(BreakStm *s) override;
  int visit(UnsafeStm *s) override;
  int visit(StructDecl *d) override;
  int visit(MethodDecl *d) override;
  int visit(ClassDecl *d) override;
  int visit(Program *p) override;
};

class GenCodeVisitor : public Visitor {
  std::ostringstream code;
  std::vector<std::string> stringLits;
  std::unordered_map<std::string, int> varOffset;
  std::unordered_map<std::string, std::string> varType;
  std::unordered_map<std::string, std::unordered_map<std::string, int>> structFieldIndex;
  std::unordered_map<std::string, int> structSize;
  std::unordered_map<std::string, long> matrixCols;
  int currentOffset = -8;
  int labelCounter = 0;
  std::string currentBreakLabel;
  std::string currentRetLabel;
  [[maybe_unused]] bool optimize;
  bool trackLoc = false;
  int curLine = -1;

  void emitLoc(int line);
  std::string newLabel(const std::string &base);
  void emitMatrixLinearIndex(MatrixAccessExp *e);
  void collectStrings(Program *p);
  int countVars(Stm *s);
  std::string exprType(Exp *e) const;

public:
  explicit GenCodeVisitor(bool optimize = true) : optimize(optimize) {}
  void enableLoc(bool on) { trackLoc = on; }

  void generar(Program *p);
  std::string render();
  void write(std::ostream &out);

  int visit(NumberExp *e) override;
  int visit(BoolExp *e) override;
  int visit(CharExp *e) override;
  int visit(StringExp *e) override;
  int visit(IdExp *e) override;
  int visit(BinaryExp *e) override;
  int visit(UnaryExp *e) override;
  int visit(CallExp *e) override;
  int visit(FieldExp *e) override;
  int visit(CastExp *e) override;
  int visit(AddressOfExp *e) override;
  int visit(DerefExp *e) override;
  int visit(NewArrayExp *e) override;
  int visit(ArrayAccessExp *e) override;
  int visit(NewMatrixExp *e) override;
  int visit(MatrixAccessExp *e) override;
  int visit(Block *s) override;
  int visit(VarDecStm *s) override;
  int visit(AssignStm *s) override;
  int visit(ExprStm *s) override;
  int visit(PrintStm *s) override;
  int visit(IfStm *s) override;
  int visit(WhileStm *s) override;
  int visit(DoWhileStm *s) override;
  int visit(ForStm *s) override;
  int visit(ReturnStm *s) override;
  int visit(BreakStm *s) override;
  int visit(UnsafeStm *s) override;
  int visit(StructDecl *d) override;
  int visit(MethodDecl *d) override;
  int visit(ClassDecl *d) override;
  int visit(Program *p) override;
};

#endif
