#include "visitor.h"
#include "ast.h"
#include "errors.h"

namespace {
bool isNumeric(const std::string &t) { return t == "int" || t == "char"; }
bool isPrintable(const std::string &t) {
  return t == "int" || t == "char" || t == "bool" || t == "string";
}
bool assignable(const std::string &to, const std::string &from) {
  if (to == from) return true;
  if (to == "int" && from == "char") return true;
  return false;
}
}

std::string TypeCheckerVisitor::typeOf(Exp *e) {
  e->accept(this);
  e->inferredType = lastType;
  return lastType;
}

bool TypeCheckerVisitor::isType(const std::string &t) const {
  return t == "int" || t == "char" || t == "bool" || t == "string";
}

int TypeCheckerVisitor::visit(NumberExp *e) { e->inferredType = lastType = "int"; return 0; }
int TypeCheckerVisitor::visit(BoolExp *e) { e->inferredType = lastType = "bool"; return 0; }
int TypeCheckerVisitor::visit(CharExp *e) { e->inferredType = lastType = "char"; return 0; }
int TypeCheckerVisitor::visit(StringExp *e) { e->inferredType = lastType = "string"; return 0; }

int TypeCheckerVisitor::visit(IdExp *e) {
  std::string t;
  if (!env.lookup(e->name, t))
    throw CompileError("semantico", "variable no declarada: '" + e->name + "'", e->line, e->col);
  e->inferredType = lastType = t;
  return 0;
}

int TypeCheckerVisitor::visit(BinaryExp *e) {
  std::string lt = typeOf(e->left);
  std::string rt = typeOf(e->right);
  switch (e->op) {
    case ADD_OP: case SUB_OP: case MUL_OP: case DIV_OP: case MOD_OP:
      if (!isNumeric(lt) || !isNumeric(rt))
        throw CompileError("semantico", std::string("operador '") + binaryOpName(e->op) +
                           "' requiere operandos numericos", e->line, e->col);
      lastType = "int";
      break;
    case LT_OP: case GT_OP: case LE_OP: case GE_OP:
      if (!isNumeric(lt) || !isNumeric(rt))
        throw CompileError("semantico", std::string("comparacion '") + binaryOpName(e->op) +
                           "' requiere operandos numericos", e->line, e->col);
      lastType = "bool";
      break;
    case EQ_OP: case NE_OP:
      if (!((isNumeric(lt) && isNumeric(rt)) || (lt == rt)))
        throw CompileError("semantico", "operandos incompatibles en '" +
                           std::string(binaryOpName(e->op)) + "'", e->line, e->col);
      lastType = "bool";
      break;
    case AND_OP: case OR_OP:
      if (lt != "bool" || rt != "bool")
        throw CompileError("semantico", std::string("operador '") + binaryOpName(e->op) +
                           "' requiere operandos bool", e->line, e->col);
      lastType = "bool";
      break;
  }
  e->inferredType = lastType;
  return 0;
}

int TypeCheckerVisitor::visit(UnaryExp *e) {
  std::string t = typeOf(e->operand);
  if (e->op == NEG_OP) {
    if (!isNumeric(t)) throw CompileError("semantico", "'-' unario requiere numerico", e->line, e->col);
    lastType = "int";
  } else {
    if (t != "bool") throw CompileError("semantico", "'!' requiere bool", e->line, e->col);
    lastType = "bool";
  }
  e->inferredType = lastType;
  return 0;
}

int TypeCheckerVisitor::visit(CallExp *e) {
  auto it = methods.find(e->name);
  if (it == methods.end())
    throw CompileError("semantico", "metodo no declarado: '" + e->name + "'", e->line, e->col);
  MethodDecl *m = it->second;
  if (e->args.size() != m->params.size())
    throw CompileError("semantico", "la llamada a '" + e->name + "' espera " +
                       std::to_string(m->params.size()) + " argumentos, se dieron " +
                       std::to_string(e->args.size()), e->line, e->col);
  for (size_t i = 0; i < e->args.size(); i++) {
    std::string at = typeOf(e->args[i]);
    if (!assignable(m->params[i].type, at))
      throw CompileError("semantico", "argumento " + std::to_string(i + 1) + " de '" + e->name +
                         "' espera " + m->params[i].type + " pero recibe " + at, e->line, e->col);
  }
  e->inferredType = lastType = m->retType;
  return 0;
}

int TypeCheckerVisitor::visit(Block *s) {
  env.add_level();
  for (auto st : s->stms) st->accept(this);
  env.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(VarDecStm *s) {
  if (env.exists_in_current(s->name))
    throw CompileError("semantico", "redeclaracion de '" + s->name + "' en el mismo scope",
                       s->line, s->col);
  if (s->type == "void")
    throw CompileError("semantico", "una variable no puede ser void", s->line, s->col);
  if (!isType(s->type))
    throw CompileError("semantico", "tipo desconocido: '" + s->type + "'", s->line, s->col);
  if (s->init) {
    std::string it = typeOf(s->init);
    if (!assignable(s->type, it))
      throw CompileError("semantico", "no se puede asignar " + it + " a variable " + s->type,
                         s->line, s->col);
  }
  env.add_var(s->name, s->type);
  return 0;
}

int TypeCheckerVisitor::visit(AssignStm *s) {
  std::string tt;
  auto id = dynamic_cast<IdExp *>(s->target);
  if (!id)
    throw CompileError("semantico", "el lado izquierdo de '=' no es asignable", s->line, s->col);
  if (!env.lookup(id->name, tt))
    throw CompileError("semantico", "variable no declarada: '" + id->name + "'", s->line, s->col);
  id->inferredType = tt;
  std::string vt = typeOf(s->value);
  if (!assignable(tt, vt))
    throw CompileError("semantico", "no se puede asignar " + vt + " a " + tt, s->line, s->col);
  return 0;
}

int TypeCheckerVisitor::visit(ExprStm *s) { typeOf(s->e); return 0; }

int TypeCheckerVisitor::visit(PrintStm *s) {
  std::string t = typeOf(s->e);
  if (!isPrintable(t))
    throw CompileError("semantico", "Console.WriteLine no puede imprimir " + t, s->line, s->col);
  return 0;
}

int TypeCheckerVisitor::visit(IfStm *s) {
  if (typeOf(s->cond) != "bool")
    throw CompileError("semantico", "la condicion de 'if' debe ser bool", s->line, s->col);
  s->thenB->accept(this);
  if (s->elseB) s->elseB->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(WhileStm *s) {
  if (typeOf(s->cond) != "bool")
    throw CompileError("semantico", "la condicion de 'while' debe ser bool", s->line, s->col);
  loopDepth++;
  s->body->accept(this);
  loopDepth--;
  return 0;
}

int TypeCheckerVisitor::visit(DoWhileStm *s) {
  loopDepth++;
  s->body->accept(this);
  loopDepth--;
  if (typeOf(s->cond) != "bool")
    throw CompileError("semantico", "la condicion de 'do-while' debe ser bool", s->line, s->col);
  return 0;
}

int TypeCheckerVisitor::visit(ForStm *s) {
  env.add_level();
  if (s->init) s->init->accept(this);
  if (s->cond && typeOf(s->cond) != "bool")
    throw CompileError("semantico", "la condicion de 'for' debe ser bool", s->line, s->col);
  if (s->update) s->update->accept(this);
  loopDepth++;
  s->body->accept(this);
  loopDepth--;
  env.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(ReturnStm *s) {
  if (s->e) {
    std::string t = typeOf(s->e);
    if (currentReturnType == "void")
      throw CompileError("semantico", "un metodo void no puede retornar un valor", s->line, s->col);
    if (!assignable(currentReturnType, t))
      throw CompileError("semantico", "return de tipo " + t + " incompatible con " +
                         currentReturnType, s->line, s->col);
  } else if (currentReturnType != "void") {
    throw CompileError("semantico", "return sin valor en metodo " + currentReturnType,
                       s->line, s->col);
  }
  return 0;
}

int TypeCheckerVisitor::visit(BreakStm *s) {
  if (loopDepth == 0)
    throw CompileError("semantico", "'break' fuera de un bucle", s->line, s->col);
  return 0;
}

int TypeCheckerVisitor::visit(MethodDecl *d) {
  currentReturnType = d->retType;
  env.add_level();
  for (auto &p : d->params) {
    if (p.type == "void")
      throw CompileError("semantico", "un parametro no puede ser void", 0, 0);
    if (!isType(p.type))
      throw CompileError("semantico", "tipo de parametro desconocido: '" + p.type + "'", 0, 0);
    env.add_var(p.name, p.type);
  }
  d->body->accept(this);
  env.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(ClassDecl *d) {
  for (auto m : d->methods) m->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(Program *p) {
  for (auto c : p->classes)
    for (auto m : c->methods) {
      if (methods.count(m->name))
        throw CompileError("semantico", "metodo duplicado: '" + m->name + "'", 0, 0);
      methods[m->name] = m;
    }
  auto it = methods.find("Main");
  if (it == methods.end())
    throw CompileError("semantico", "no se encontro el metodo de entrada 'Main'", 0, 0);
  MethodDecl *main = it->second;
  if (!main->isStatic || main->retType != "int" || !main->params.empty())
    throw CompileError("semantico", "'Main' debe ser 'static int Main()' sin parametros", 0, 0);
  for (auto c : p->classes) c->accept(this);
  return 0;
}

void TypeCheckerVisitor::check(Program *p) { p->accept(this); }
