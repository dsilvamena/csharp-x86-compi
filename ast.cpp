#include "ast.h"
#include "errors.h"
#include "visitor.h"
#include <sstream>

const char *binaryOpName(BinaryOp op) {
  switch (op) {
    case ADD_OP: return "+";
    case SUB_OP: return "-";
    case MUL_OP: return "*";
    case DIV_OP: return "/";
    case MOD_OP: return "%";
    case LT_OP: return "<";
    case GT_OP: return ">";
    case LE_OP: return "<=";
    case GE_OP: return ">=";
    case EQ_OP: return "==";
    case NE_OP: return "!=";
    case AND_OP: return "&&";
    case OR_OP: return "||";
  }
  return "?";
}

int NumberExp::accept(Visitor *v) { return v->visit(this); }
int BoolExp::accept(Visitor *v) { return v->visit(this); }
int CharExp::accept(Visitor *v) { return v->visit(this); }
int StringExp::accept(Visitor *v) { return v->visit(this); }
int IdExp::accept(Visitor *v) { return v->visit(this); }
int BinaryExp::accept(Visitor *v) { return v->visit(this); }
int UnaryExp::accept(Visitor *v) { return v->visit(this); }
int CallExp::accept(Visitor *v) { return v->visit(this); }
int FieldExp::accept(Visitor *v) { return v->visit(this); }
int CastExp::accept(Visitor *v) { return v->visit(this); }
int NewArrayExp::accept(Visitor *v) { return v->visit(this); }
int ArrayAccessExp::accept(Visitor *v) { return v->visit(this); }
int NewMatrixExp::accept(Visitor *v) { return v->visit(this); }
int MatrixAccessExp::accept(Visitor *v) { return v->visit(this); }
int Block::accept(Visitor *v) { return v->visit(this); }
int VarDecStm::accept(Visitor *v) { return v->visit(this); }
int AssignStm::accept(Visitor *v) { return v->visit(this); }
int ExprStm::accept(Visitor *v) { return v->visit(this); }
int PrintStm::accept(Visitor *v) { return v->visit(this); }
int IfStm::accept(Visitor *v) { return v->visit(this); }
int WhileStm::accept(Visitor *v) { return v->visit(this); }
int DoWhileStm::accept(Visitor *v) { return v->visit(this); }
int ForStm::accept(Visitor *v) { return v->visit(this); }
int ReturnStm::accept(Visitor *v) { return v->visit(this); }
int BreakStm::accept(Visitor *v) { return v->visit(this); }
int StructDecl::accept(Visitor *v) { return v->visit(this); }
int MethodDecl::accept(Visitor *v) { return v->visit(this); }
int ClassDecl::accept(Visitor *v) { return v->visit(this); }
int Program::accept(Visitor *v) { return v->visit(this); }

namespace {
class Printer {
public:
  std::ostringstream out;
  int depth = 0;

  void line(const std::string &s) {
    for (int i = 0; i < depth; i++) out << "  ";
    out << s << "\n";
  }
  void exp(Exp *e) {
    if (!e) { line("<null>"); return; }
    if (auto n = dynamic_cast<NumberExp *>(e)) line("NumberExp " + std::to_string(n->value));
    else if (auto b = dynamic_cast<BoolExp *>(e)) line(std::string("BoolExp ") + (b->value ? "true" : "false"));
    else if (auto c = dynamic_cast<CharExp *>(e)) line("CharExp " + std::to_string(c->value));
    else if (auto s = dynamic_cast<StringExp *>(e)) line("StringExp \"" + s->value + "\"");
    else if (auto id = dynamic_cast<IdExp *>(e)) line("IdExp " + id->name);
    else if (auto b = dynamic_cast<BinaryExp *>(e)) {
      line(std::string("BinaryExp ") + binaryOpName(b->op));
      depth++; exp(b->left); exp(b->right); depth--;
    } else if (auto u = dynamic_cast<UnaryExp *>(e)) {
      line(std::string("UnaryExp ") + (u->op == NEG_OP ? "-" : "!"));
      depth++; exp(u->operand); depth--;
    } else if (auto c = dynamic_cast<CallExp *>(e)) {
      line("CallExp " + c->name);
      depth++;
      for (auto a : c->args) exp(a);
      depth--;
    } else if (auto f = dynamic_cast<FieldExp *>(e)) {
      line("FieldExp " + f->object + "." + f->field);
    } else if (auto ca = dynamic_cast<CastExp *>(e)) {
      line("CastExp (" + ca->targetType + ")");
      depth++; exp(ca->operand); depth--;
    } else if (auto n = dynamic_cast<NewArrayExp *>(e)) {
      line("NewArrayExp " + n->elemType + "[]");
      depth++; exp(n->size); depth--;
    } else if (auto a = dynamic_cast<ArrayAccessExp *>(e)) {
      line("ArrayAccessExp " + a->name);
      depth++; exp(a->index); depth--;
    } else if (auto n = dynamic_cast<NewMatrixExp *>(e)) {
      line("NewMatrixExp " + n->elemType + "[,]");
      depth++; exp(n->rows); exp(n->cols); depth--;
    } else if (auto m = dynamic_cast<MatrixAccessExp *>(e)) {
      line("MatrixAccessExp " + m->name);
      depth++; exp(m->rowIdx); exp(m->colIdx); depth--;
    }
  }
  void stm(Stm *s) {
    if (!s) { line("<null>"); return; }
    if (auto b = dynamic_cast<Block *>(s)) {
      line("Block");
      depth++;
      for (auto st : b->stms) stm(st);
      depth--;
    } else if (auto v = dynamic_cast<VarDecStm *>(s)) {
      line("VarDecStm " + v->type + " " + v->name);
      if (v->init) { depth++; exp(v->init); depth--; }
    } else if (auto a = dynamic_cast<AssignStm *>(s)) {
      line("AssignStm");
      depth++; exp(a->target); exp(a->value); depth--;
    } else if (auto e = dynamic_cast<ExprStm *>(s)) {
      line("ExprStm"); depth++; exp(e->e); depth--;
    } else if (auto p = dynamic_cast<PrintStm *>(s)) {
      line("PrintStm"); depth++; exp(p->e); depth--;
    } else if (auto i = dynamic_cast<IfStm *>(s)) {
      line("IfStm"); depth++; exp(i->cond); stm(i->thenB); stm(i->elseB); depth--;
    } else if (auto w = dynamic_cast<WhileStm *>(s)) {
      line("WhileStm"); depth++; exp(w->cond); stm(w->body); depth--;
    } else if (auto d = dynamic_cast<DoWhileStm *>(s)) {
      line("DoWhileStm"); depth++; stm(d->body); exp(d->cond); depth--;
    } else if (auto f = dynamic_cast<ForStm *>(s)) {
      line("ForStm"); depth++; stm(f->init); exp(f->cond); stm(f->update); stm(f->body); depth--;
    } else if (auto r = dynamic_cast<ReturnStm *>(s)) {
      line("ReturnStm"); if (r->e) { depth++; exp(r->e); depth--; }
    } else if (dynamic_cast<BreakStm *>(s)) {
      line("BreakStm");
    }
  }
  void method(MethodDecl *m) {
    line("MethodDecl " + m->retType + " " + m->name);
    depth++;
    for (auto &p : m->params) line("Param " + p.type + " " + p.name);
    stm(m->body);
    depth--;
  }
};
}

std::string printAst(Program *p) {
  Printer pr;
  pr.line("Program");
  pr.depth++;
  for (auto s : p->structs) {
    pr.line("StructDecl " + s->name);
    pr.depth++;
    for (auto &f : s->fields) pr.line("Field " + f.type + " " + f.name);
    pr.depth--;
  }
  for (auto c : p->classes) {
    pr.line("ClassDecl " + c->name);
    pr.depth++;
    for (auto m : c->methods) pr.method(m);
    pr.depth--;
  }
  pr.depth--;
  return pr.out.str();
}
