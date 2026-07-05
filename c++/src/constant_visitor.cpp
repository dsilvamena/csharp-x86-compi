#include "visitor.h"
#include "ast.h"

namespace {
bool evalBinary(long l, long r, BinaryOp op, long &out) {
  switch (op) {
    case ADD_OP: out = l + r; return true;
    case SUB_OP: out = l - r; return true;
    case MUL_OP: out = l * r; return true;
    case DIV_OP: if (r == 0) return false; out = l / r; return true;
    case MOD_OP: if (r == 0) return false; out = l % r; return true;
    case LT_OP: out = l < r; return true;
    case GT_OP: out = l > r; return true;
    case LE_OP: out = l <= r; return true;
    case GE_OP: out = l >= r; return true;
    case EQ_OP: out = l == r; return true;
    case NE_OP: out = l != r; return true;
    case AND_OP: out = (l != 0) && (r != 0); return true;
    case OR_OP: out = (l != 0) || (r != 0); return true;
  }
  return false;
}
}

int ConstantVisitor::visit(NumberExp *e) { e->isConstant = true; e->constantValue = e->value; return 0; }
int ConstantVisitor::visit(BoolExp *e) { e->isConstant = true; e->constantValue = e->value ? 1 : 0; return 0; }
int ConstantVisitor::visit(CharExp *e) { e->isConstant = true; e->constantValue = e->value; return 0; }
int ConstantVisitor::visit(StringExp *e) { e->isConstant = false; return 0; }
int ConstantVisitor::visit(IdExp *e) { e->isConstant = false; return 0; }

int ConstantVisitor::visit(BinaryExp *e) {
  e->left->accept(this);
  e->right->accept(this);
  long v = 0;
  if (e->left->isConstant && e->right->isConstant &&
      evalBinary(e->left->constantValue, e->right->constantValue, e->op, v)) {
    e->isConstant = true;
    e->constantValue = v;
    folded++;
  }
  return 0;
}

int ConstantVisitor::visit(UnaryExp *e) {
  e->operand->accept(this);
  if (e->operand->isConstant) {
    e->isConstant = true;
    e->constantValue = e->op == NEG_OP ? -e->operand->constantValue
                                       : (e->operand->constantValue == 0);
    folded++;
  }
  return 0;
}

int ConstantVisitor::visit(CallExp *e) { for (auto a : e->args) a->accept(this); return 0; }
int ConstantVisitor::visit(FieldExp *e) { e->isConstant = false; return 0; }

int ConstantVisitor::visit(CastExp *e) {
  e->operand->accept(this);
  e->isConstant = false;
  if (!e->operand->isConstant) return 0;
  long v = e->operand->constantValue;
  e->constantValue = (e->targetType == "char") ? (v & 0xFF) : v;  // (int): identidad
  e->isConstant = true;
  folded++;
  return 0;
}
int ConstantVisitor::visit(AddressOfExp *e) { e->isConstant = false; return 0; }
int ConstantVisitor::visit(DerefExp *e) { e->ptr->accept(this); e->isConstant = false; return 0; }
int ConstantVisitor::visit(NewArrayExp *e) { e->size->accept(this); e->isConstant = false; return 0; }
int ConstantVisitor::visit(ArrayAccessExp *e) { e->index->accept(this); e->isConstant = false; return 0; }
int ConstantVisitor::visit(NewMatrixExp *e) { e->rows->accept(this); e->cols->accept(this); e->isConstant = false; return 0; }
int ConstantVisitor::visit(MatrixAccessExp *e) { e->rowIdx->accept(this); e->colIdx->accept(this); e->isConstant = false; return 0; }
int ConstantVisitor::visit(Block *s) { for (auto st : s->stms) st->accept(this); return 0; }
int ConstantVisitor::visit(VarDecStm *s) { if (s->init) s->init->accept(this); return 0; }
int ConstantVisitor::visit(AssignStm *s) { s->target->accept(this); s->value->accept(this); return 0; }
int ConstantVisitor::visit(ExprStm *s) { s->e->accept(this); return 0; }
int ConstantVisitor::visit(PrintStm *s) { s->e->accept(this); return 0; }
int ConstantVisitor::visit(IfStm *s) { s->cond->accept(this); s->thenB->accept(this); if (s->elseB) s->elseB->accept(this); return 0; }
int ConstantVisitor::visit(WhileStm *s) { s->cond->accept(this); s->body->accept(this); return 0; }
int ConstantVisitor::visit(DoWhileStm *s) { s->body->accept(this); s->cond->accept(this); return 0; }
int ConstantVisitor::visit(ForStm *s) { if (s->init) s->init->accept(this); if (s->cond) s->cond->accept(this); if (s->update) s->update->accept(this); s->body->accept(this); return 0; }
int ConstantVisitor::visit(ReturnStm *s) { if (s->e) s->e->accept(this); return 0; }
int ConstantVisitor::visit(BreakStm *) { return 0; }
int ConstantVisitor::visit(UnsafeStm *s) { s->body->accept(this); return 0; }
int ConstantVisitor::visit(StructDecl *) { return 0; }
int ConstantVisitor::visit(MethodDecl *d) { d->body->accept(this); return 0; }
int ConstantVisitor::visit(ClassDecl *d) { for (auto m : d->methods) m->accept(this); return 0; }
int ConstantVisitor::visit(Program *p) { for (auto *s : p->structs) s->accept(this); for (auto c : p->classes) c->accept(this); return 0; }
