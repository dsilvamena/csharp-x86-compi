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
int AddressOfExp::accept(Visitor *v) { return v->visit(this); }
int DerefExp::accept(Visitor *v) { return v->visit(this); }
int NewArrayExp::accept(Visitor *v) { return v->visit(this); }
int ArrayAccessExp::accept(Visitor *v) { return v->visit(this); }
int NewMatrixExp::accept(Visitor *v) { return v->visit(this); }
int MatrixAccessExp::accept(Visitor *v) { return v->visit(this); }
int LambdaExp::accept(Visitor *) {
  throw CompileError("semantico",
      "lambda en posicion no soportada: solo se admite 'var f = (T x) => expr;' "
      "invocada como f(...)",
      line, col);
}
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
int UnsafeStm::accept(Visitor *v) { return v->visit(this); }
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
    } else if (auto ao = dynamic_cast<AddressOfExp *>(e)) {
      line("AddressOfExp &" + ao->name);
    } else if (auto de = dynamic_cast<DerefExp *>(e)) {
      line("DerefExp *");
      depth++; exp(de->ptr); depth--;
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
    } else if (auto u = dynamic_cast<UnsafeStm *>(s)) {
      line("UnsafeStm"); depth++; stm(u->body); depth--;
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

class DotPrinter {
public:
  std::ostringstream out;
  int nextId = 0;

  static std::string esc(const std::string &s) {
    std::string r;
    for (char ch : s) {
      if (ch == '\\') r += "\\\\";
      else if (ch == '"') r += "\\\"";
      else if (ch == '\n') r += "\\n";
      else if (ch == '\r') r += "\\r";
      else if (ch == '\t') r += "\\t";
      else r += ch;
    }
    return r;
  }

  int node(const std::string &label) {
    int id = nextId++;
    out << "  n" << id << " [label=\"" << esc(label) << "\"];\n";
    return id;
  }

  void edge(int from, int to, const std::string &label = "") {
    out << "  n" << from << " -> n" << to;
    if (!label.empty()) out << " [label=\"" << esc(label) << "\"]";
    out << ";\n";
  }

  int nullNode() {
    return node("<null>");
  }

  int exp(Exp *e) {
    if (!e) return nullNode();
    if (auto n = dynamic_cast<NumberExp *>(e)) return node("NumberExp " + std::to_string(n->value));
    if (auto b = dynamic_cast<BoolExp *>(e)) return node(std::string("BoolExp ") + (b->value ? "true" : "false"));
    if (auto c = dynamic_cast<CharExp *>(e)) return node("CharExp " + std::to_string(c->value));
    if (auto s = dynamic_cast<StringExp *>(e)) return node("StringExp \"" + s->value + "\"");
    if (auto id = dynamic_cast<IdExp *>(e)) return node("IdExp " + id->name);
    if (auto b = dynamic_cast<BinaryExp *>(e)) {
      int id = node(std::string("BinaryExp ") + binaryOpName(b->op));
      int left = exp(b->left), right = exp(b->right);
      edge(id, left, "left");
      edge(id, right, "right");
      return id;
    }
    if (auto u = dynamic_cast<UnaryExp *>(e)) {
      int id = node(std::string("UnaryExp ") + (u->op == NEG_OP ? "-" : "!"));
      edge(id, exp(u->operand), "operand");
      return id;
    }
    if (auto c = dynamic_cast<CallExp *>(e)) {
      int id = node("CallExp " + c->name);
      for (auto a : c->args) edge(id, exp(a), "arg");
      return id;
    }
    if (auto f = dynamic_cast<FieldExp *>(e)) return node("FieldExp " + f->object + "." + f->field);
    if (auto ca = dynamic_cast<CastExp *>(e)) {
      int id = node("CastExp (" + ca->targetType + ")");
      edge(id, exp(ca->operand), "operand");
      return id;
    }
    if (auto ao = dynamic_cast<AddressOfExp *>(e)) return node("AddressOfExp &" + ao->name);
    if (auto de = dynamic_cast<DerefExp *>(e)) {
      int id = node("DerefExp *");
      edge(id, exp(de->ptr), "ptr");
      return id;
    }
    if (auto n = dynamic_cast<NewArrayExp *>(e)) {
      int id = node("NewArrayExp " + n->elemType + "[]");
      edge(id, exp(n->size), "size");
      return id;
    }
    if (auto a = dynamic_cast<ArrayAccessExp *>(e)) {
      int id = node("ArrayAccessExp " + a->name);
      edge(id, exp(a->index), "index");
      return id;
    }
    if (auto n = dynamic_cast<NewMatrixExp *>(e)) {
      int id = node("NewMatrixExp " + n->elemType + "[,]");
      edge(id, exp(n->rows), "rows");
      edge(id, exp(n->cols), "cols");
      return id;
    }
    if (auto m = dynamic_cast<MatrixAccessExp *>(e)) {
      int id = node("MatrixAccessExp " + m->name);
      edge(id, exp(m->rowIdx), "row");
      edge(id, exp(m->colIdx), "col");
      return id;
    }
    if (auto l = dynamic_cast<LambdaExp *>(e)) {
      int id = node("LambdaExp");
      for (auto &p : l->params) edge(id, node("Param " + p.type + " " + p.name), "param");
      edge(id, exp(l->body), "body");
      return id;
    }
    return node("<unknown exp>");
  }

  int stm(Stm *s) {
    if (!s) return nullNode();
    if (auto b = dynamic_cast<Block *>(s)) {
      int id = node("Block");
      for (auto st : b->stms) edge(id, stm(st));
      return id;
    }
    if (auto v = dynamic_cast<VarDecStm *>(s)) {
      int id = node("VarDecStm " + v->type + " " + v->name);
      if (v->init) edge(id, exp(v->init), "init");
      return id;
    }
    if (auto a = dynamic_cast<AssignStm *>(s)) {
      int id = node("AssignStm");
      edge(id, exp(a->target), "target");
      edge(id, exp(a->value), "value");
      return id;
    }
    if (auto e = dynamic_cast<ExprStm *>(s)) {
      int id = node("ExprStm");
      edge(id, exp(e->e));
      return id;
    }
    if (auto p = dynamic_cast<PrintStm *>(s)) {
      int id = node("PrintStm");
      edge(id, exp(p->e));
      return id;
    }
    if (auto i = dynamic_cast<IfStm *>(s)) {
      int id = node("IfStm");
      edge(id, exp(i->cond), "cond");
      edge(id, stm(i->thenB), "then");
      edge(id, stm(i->elseB), "else");
      return id;
    }
    if (auto w = dynamic_cast<WhileStm *>(s)) {
      int id = node("WhileStm");
      edge(id, exp(w->cond), "cond");
      edge(id, stm(w->body), "body");
      return id;
    }
    if (auto d = dynamic_cast<DoWhileStm *>(s)) {
      int id = node("DoWhileStm");
      edge(id, stm(d->body), "body");
      edge(id, exp(d->cond), "cond");
      return id;
    }
    if (auto f = dynamic_cast<ForStm *>(s)) {
      int id = node("ForStm");
      edge(id, stm(f->init), "init");
      edge(id, exp(f->cond), "cond");
      edge(id, stm(f->update), "update");
      edge(id, stm(f->body), "body");
      return id;
    }
    if (auto r = dynamic_cast<ReturnStm *>(s)) {
      int id = node("ReturnStm");
      if (r->e) edge(id, exp(r->e));
      return id;
    }
    if (dynamic_cast<BreakStm *>(s)) return node("BreakStm");
    if (auto u = dynamic_cast<UnsafeStm *>(s)) {
      int id = node("UnsafeStm");
      edge(id, stm(u->body), "body");
      return id;
    }
    return node("<unknown stm>");
  }

  int method(MethodDecl *m) {
    int id = node("MethodDecl " + m->retType + " " + m->name);
    for (auto &p : m->params) edge(id, node("Param " + p.type + " " + p.name), "param");
    edge(id, stm(m->body), "body");
    return id;
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

std::string astToDot(Program *p) {
  DotPrinter pr;
  pr.out << "digraph AST {\n";
  pr.out << "  node [shape=box];\n";
  int root = pr.node("Program");
  for (auto s : p->structs) {
    int sid = pr.node("StructDecl " + s->name);
    pr.edge(root, sid);
    for (auto &f : s->fields) pr.edge(sid, pr.node("Field " + f.type + " " + f.name));
  }
  for (auto c : p->classes) {
    int cid = pr.node("ClassDecl " + c->name);
    pr.edge(root, cid);
    for (auto m : c->methods) pr.edge(cid, pr.method(m));
  }
  pr.out << "}\n";
  return pr.out.str();
}
