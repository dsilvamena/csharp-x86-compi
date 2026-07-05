#include "ast.h"

#include <map>
#include <string>
#include <vector>

namespace {

std::string inferRet(Exp *body) {
  if (dynamic_cast<BoolExp *>(body)) return "bool";
  if (dynamic_cast<StringExp *>(body)) return "string";
  if (auto u = dynamic_cast<UnaryExp *>(body)) return u->op == NOT_OP ? "bool" : "int";
  if (auto b = dynamic_cast<BinaryExp *>(body)) {
    switch (b->op) {
      case LT_OP: case GT_OP: case LE_OP: case GE_OP:
      case EQ_OP: case NE_OP: case AND_OP: case OR_OP: return "bool";
      default: return "int";
    }
  }
  return "int";
}

struct Lifter {
  int counter;
  std::map<std::string, std::string> bindings;  // variable -> función generada
  std::vector<MethodDecl *> generated;
  explicit Lifter(int c) : counter(c) {}

  void collectStm(Stm *s) {
    if (!s) return;
    if (auto b = dynamic_cast<Block *>(s)) { for (auto st : b->stms) collectStm(st); }
    else if (auto vd = dynamic_cast<VarDecStm *>(s)) {
      if (auto lam = dynamic_cast<LambdaExp *>(vd->init)) generateFor(vd->name, lam);
    }
    else if (auto ifs = dynamic_cast<IfStm *>(s)) { collectStm(ifs->thenB); collectStm(ifs->elseB); }
    else if (auto ws = dynamic_cast<WhileStm *>(s)) collectStm(ws->body);
    else if (auto dws = dynamic_cast<DoWhileStm *>(s)) collectStm(dws->body);
    else if (auto fs = dynamic_cast<ForStm *>(s)) { collectStm(fs->init); collectStm(fs->body); }
    else if (auto us = dynamic_cast<UnsafeStm *>(s)) collectStm(us->body);
  }

  void generateFor(const std::string &varName, LambdaExp *lam) {
    std::string gen = "__lambda" + std::to_string(counter++);
    auto *m = new MethodDecl();
    m->isStatic = true;
    m->name = gen;
    m->params = lam->params;
    m->retType = inferRet(lam->body);
    auto *body = new Block();
    auto *ret = new ReturnStm(lam->body);
    ret->line = lam->line; ret->col = lam->col;
    body->stms.push_back(ret);
    m->body = body;
    lam->body = nullptr;
    generated.push_back(m);
    bindings[varName] = gen;
  }

  void rewriteExp(Exp *e) {
    if (!e) return;
    if (auto b = dynamic_cast<BinaryExp *>(e)) { rewriteExp(b->left); rewriteExp(b->right); }
    else if (auto u = dynamic_cast<UnaryExp *>(e)) rewriteExp(u->operand);
    else if (auto c = dynamic_cast<CallExp *>(e)) {
      for (auto a : c->args) rewriteExp(a);
      auto it = bindings.find(c->name);
      if (it != bindings.end()) c->name = it->second;
    }
    else if (auto ca = dynamic_cast<CastExp *>(e)) rewriteExp(ca->operand);
    else if (auto d = dynamic_cast<DerefExp *>(e)) rewriteExp(d->ptr);
    else if (auto na = dynamic_cast<NewArrayExp *>(e)) rewriteExp(na->size);
    else if (auto aa = dynamic_cast<ArrayAccessExp *>(e)) rewriteExp(aa->index);
    else if (auto nm = dynamic_cast<NewMatrixExp *>(e)) { rewriteExp(nm->rows); rewriteExp(nm->cols); }
    else if (auto ma = dynamic_cast<MatrixAccessExp *>(e)) { rewriteExp(ma->rowIdx); rewriteExp(ma->colIdx); }
  }

  void rewriteStm(Stm *s) {
    if (!s) return;
    if (auto b = dynamic_cast<Block *>(s)) { for (auto st : b->stms) rewriteStm(st); }
    else if (auto vd = dynamic_cast<VarDecStm *>(s)) rewriteExp(vd->init);
    else if (auto as = dynamic_cast<AssignStm *>(s)) { rewriteExp(as->target); rewriteExp(as->value); }
    else if (auto es = dynamic_cast<ExprStm *>(s)) rewriteExp(es->e);
    else if (auto ps = dynamic_cast<PrintStm *>(s)) rewriteExp(ps->e);
    else if (auto ifs = dynamic_cast<IfStm *>(s)) { rewriteExp(ifs->cond); rewriteStm(ifs->thenB); rewriteStm(ifs->elseB); }
    else if (auto ws = dynamic_cast<WhileStm *>(s)) { rewriteExp(ws->cond); rewriteStm(ws->body); }
    else if (auto dws = dynamic_cast<DoWhileStm *>(s)) { rewriteStm(dws->body); rewriteExp(dws->cond); }
    else if (auto fs = dynamic_cast<ForStm *>(s)) { rewriteStm(fs->init); rewriteExp(fs->cond); rewriteStm(fs->update); rewriteStm(fs->body); }
    else if (auto rs = dynamic_cast<ReturnStm *>(s)) rewriteExp(rs->e);
    else if (auto us = dynamic_cast<UnsafeStm *>(s)) rewriteStm(us->body);
  }

  void stripBlock(Block *b) {
    std::vector<Stm *> keep;
    for (auto st : b->stms) {
      auto vd = dynamic_cast<VarDecStm *>(st);
      if (vd && bindings.count(vd->name) && dynamic_cast<LambdaExp *>(vd->init)) {
        delete st;
        continue;
      }
      stripStm(st);
      keep.push_back(st);
    }
    b->stms.swap(keep);
  }

  void stripStm(Stm *s) {
    if (auto b = dynamic_cast<Block *>(s)) stripBlock(b);
    else if (auto ifs = dynamic_cast<IfStm *>(s)) { stripBlock(ifs->thenB); if (ifs->elseB) stripBlock(ifs->elseB); }
    else if (auto ws = dynamic_cast<WhileStm *>(s)) stripBlock(ws->body);
    else if (auto dws = dynamic_cast<DoWhileStm *>(s)) stripBlock(dws->body);
    else if (auto fs = dynamic_cast<ForStm *>(s)) stripBlock(fs->body);
    else if (auto us = dynamic_cast<UnsafeStm *>(s)) stripBlock(us->body);
  }
};

}  // namespace

void liftLambdas(Program *p) {
  int counter = 0;
  for (auto c : p->classes) {
    std::vector<MethodDecl *> originals = c->methods;
    std::vector<MethodDecl *> allGen;
    for (auto m : originals) {
      Lifter lf(counter);
      lf.collectStm(m->body);
      if (lf.bindings.empty()) continue;
      lf.rewriteStm(m->body);
      for (auto g : lf.generated) lf.rewriteStm(g->body);
      lf.stripStm(m->body);
      for (auto g : lf.generated) allGen.push_back(g);
      counter = lf.counter;
    }
    for (auto g : allGen) c->methods.push_back(g);
  }
}
