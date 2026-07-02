#include "visitor.h"
#include "ast.h"
#include <algorithm>

int SethiUllmanVisitor::visit(NumberExp *e) { e->label = 1; return 0; }
int SethiUllmanVisitor::visit(BoolExp *e) { e->label = 1; return 0; }
int SethiUllmanVisitor::visit(CharExp *e) { e->label = 1; return 0; }
int SethiUllmanVisitor::visit(StringExp *e) { e->label = 1; return 0; }
int SethiUllmanVisitor::visit(IdExp *e) { e->label = 1; return 0; }

int SethiUllmanVisitor::visit(BinaryExp *e) {
  e->left->accept(this);
  e->right->accept(this);
  e->label = e->left->label == e->right->label
             ? e->left->label + 1
             : std::max(e->left->label, e->right->label);
  return 0;
}

int SethiUllmanVisitor::visit(UnaryExp *e) { e->operand->accept(this); e->label = e->operand->label; return 0; }
int SethiUllmanVisitor::visit(CallExp *e) { for (auto a : e->args) a->accept(this); e->label = 1; return 0; }
int SethiUllmanVisitor::visit(FieldExp *e) { e->label = 1; return 0; }
int SethiUllmanVisitor::visit(NewArrayExp *e) { e->size->accept(this); e->label = 1; return 0; }
int SethiUllmanVisitor::visit(ArrayAccessExp *e) { e->index->accept(this); e->label = 1; return 0; }
int SethiUllmanVisitor::visit(NewMatrixExp *e) { e->rows->accept(this); e->cols->accept(this); e->label = 1; return 0; }
int SethiUllmanVisitor::visit(MatrixAccessExp *e) { e->rowIdx->accept(this); e->colIdx->accept(this); e->label = 1; return 0; }
int SethiUllmanVisitor::visit(Block *s) { for (auto st : s->stms) st->accept(this); return 0; }
int SethiUllmanVisitor::visit(VarDecStm *s) { if (s->init) s->init->accept(this); return 0; }
int SethiUllmanVisitor::visit(AssignStm *s) { s->target->accept(this); s->value->accept(this); return 0; }
int SethiUllmanVisitor::visit(ExprStm *s) { s->e->accept(this); return 0; }
int SethiUllmanVisitor::visit(PrintStm *s) { s->e->accept(this); return 0; }
int SethiUllmanVisitor::visit(IfStm *s) { s->cond->accept(this); s->thenB->accept(this); if (s->elseB) s->elseB->accept(this); return 0; }
int SethiUllmanVisitor::visit(WhileStm *s) { s->cond->accept(this); s->body->accept(this); return 0; }
int SethiUllmanVisitor::visit(DoWhileStm *s) { s->body->accept(this); s->cond->accept(this); return 0; }
int SethiUllmanVisitor::visit(ForStm *s) { if (s->init) s->init->accept(this); if (s->cond) s->cond->accept(this); if (s->update) s->update->accept(this); s->body->accept(this); return 0; }
int SethiUllmanVisitor::visit(ReturnStm *s) { if (s->e) s->e->accept(this); return 0; }
int SethiUllmanVisitor::visit(BreakStm *) { return 0; }
int SethiUllmanVisitor::visit(StructDecl *) { return 0; }
int SethiUllmanVisitor::visit(MethodDecl *d) { d->body->accept(this); return 0; }
int SethiUllmanVisitor::visit(ClassDecl *d) { for (auto m : d->methods) m->accept(this); return 0; }
int SethiUllmanVisitor::visit(Program *p) { for (auto *s : p->structs) s->accept(this); for (auto c : p->classes) c->accept(this); return 0; }
