#include "visitor.h"
#include "ast.h"
#include "errors.h"

namespace {
const char *ARG_REGS[6] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

int align16(int n) { return (n + 15) & ~15; }

std::string escapeForAsm(const std::string &s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\t': out += "\\t"; break;
      case '\r': out += "\\r"; break;
      default: out += c;
    }
  }
  return out;
}
}

std::string GenCodeVisitor::newLabel(const std::string &base) {
  return ".L" + base + std::to_string(labelCounter++);
}

std::string GenCodeVisitor::exprType(Exp *e) const { return e->inferredType; }

namespace {
void collectExp(Exp *e, std::vector<std::string> &lits);

void collectStm(Stm *s, std::vector<std::string> &lits) {
  if (!s) return;
  if (auto b = dynamic_cast<Block *>(s)) { for (auto st : b->stms) collectStm(st, lits); }
  else if (auto vd = dynamic_cast<VarDecStm *>(s)) collectExp(vd->init, lits);
  else if (auto as = dynamic_cast<AssignStm *>(s)) { collectExp(as->target, lits); collectExp(as->value, lits); }
  else if (auto es = dynamic_cast<ExprStm *>(s)) collectExp(es->e, lits);
  else if (auto ps = dynamic_cast<PrintStm *>(s)) collectExp(ps->e, lits);
  else if (auto ifs = dynamic_cast<IfStm *>(s)) { collectExp(ifs->cond, lits); collectStm(ifs->thenB, lits); collectStm(ifs->elseB, lits); }
  else if (auto ws = dynamic_cast<WhileStm *>(s)) { collectExp(ws->cond, lits); collectStm(ws->body, lits); }
  else if (auto dws = dynamic_cast<DoWhileStm *>(s)) { collectStm(dws->body, lits); collectExp(dws->cond, lits); }
  else if (auto fs = dynamic_cast<ForStm *>(s)) { collectStm(fs->init, lits); collectExp(fs->cond, lits); collectStm(fs->update, lits); collectStm(fs->body, lits); }
  else if (auto rs = dynamic_cast<ReturnStm *>(s)) collectExp(rs->e, lits);
}

void collectExp(Exp *e, std::vector<std::string> &lits) {
  if (!e) return;
  if (auto s = dynamic_cast<StringExp *>(e)) {
    s->labelId = (int)lits.size();
    lits.push_back(s->value);
  } else if (auto b = dynamic_cast<BinaryExp *>(e)) {
    collectExp(b->left, lits);
    collectExp(b->right, lits);
  } else if (auto u = dynamic_cast<UnaryExp *>(e)) {
    collectExp(u->operand, lits);
  } else if (auto c = dynamic_cast<CallExp *>(e)) {
    for (auto a : c->args) collectExp(a, lits);
  } else if (auto ca = dynamic_cast<CastExp *>(e)) {
    collectExp(ca->operand, lits);
  } else if (auto na = dynamic_cast<NewArrayExp *>(e)) {
    collectExp(na->size, lits);
  } else if (auto aa = dynamic_cast<ArrayAccessExp *>(e)) {
    collectExp(aa->index, lits);
  } else if (auto nm = dynamic_cast<NewMatrixExp *>(e)) {
    collectExp(nm->rows, lits);
    collectExp(nm->cols, lits);
  } else if (auto ma = dynamic_cast<MatrixAccessExp *>(e)) {
    collectExp(ma->rowIdx, lits);
    collectExp(ma->colIdx, lits);
  }
}
}

void GenCodeVisitor::collectStrings(Program *p) {
  for (auto c : p->classes)
    for (auto m : c->methods)
      collectStm(m->body, stringLits);
}

int GenCodeVisitor::countVars(Stm *s) {
  if (!s) return 0;
  if (auto b = dynamic_cast<Block *>(s)) { int n = 0; for (auto st : b->stms) n += countVars(st); return n; }
  if (dynamic_cast<VarDecStm *>(s)) return 1;
  if (auto ifs = dynamic_cast<IfStm *>(s)) return countVars(ifs->thenB) + countVars(ifs->elseB);
  if (auto ws = dynamic_cast<WhileStm *>(s)) return countVars(ws->body);
  if (auto dws = dynamic_cast<DoWhileStm *>(s)) return countVars(dws->body);
  if (auto fs = dynamic_cast<ForStm *>(s)) return countVars(fs->init) + countVars(fs->body);
  return 0;
}

int GenCodeVisitor::visit(NumberExp *e) { code << "  movq $" << e->value << ", %rax\n"; return 0; }
int GenCodeVisitor::visit(BoolExp *e) { code << "  movq $" << (e->value ? 1 : 0) << ", %rax\n"; return 0; }
int GenCodeVisitor::visit(CharExp *e) { code << "  movq $" << e->value << ", %rax\n"; return 0; }
int GenCodeVisitor::visit(StringExp *e) { code << "  leaq .Lstr" << e->labelId << "(%rip), %rax\n"; return 0; }
int GenCodeVisitor::visit(IdExp *e) { code << "  movq " << varOffset[e->name] << "(%rbp), %rax\n"; return 0; }

int GenCodeVisitor::visit(BinaryExp *e) {
  if (optimize && e->isConstant) {
    code << "  movq $" << e->constantValue << ", %rax\n";
    return 0;
  }
  if (e->op == AND_OP || e->op == OR_OP) {
    std::string lend = newLabel("bool_end");
    std::string lset = newLabel("bool_set");
    e->left->accept(this);
    code << "  cmpq $0, %rax\n";
    if (e->op == AND_OP) {
      code << "  je " << lset << "\n";
      e->right->accept(this);
      code << "  cmpq $0, %rax\n  je " << lset << "\n";
      code << "  movq $1, %rax\n  jmp " << lend << "\n";
      code << lset << ":\n  movq $0, %rax\n";
    } else {
      code << "  jne " << lset << "\n";
      e->right->accept(this);
      code << "  cmpq $0, %rax\n  jne " << lset << "\n";
      code << "  movq $0, %rax\n  jmp " << lend << "\n";
      code << lset << ":\n  movq $1, %rax\n";
    }
    code << lend << ":\n";
    return 0;
  }
  Exp *first = e->left, *second = e->right;
  bool commutative = (e->op == ADD_OP || e->op == MUL_OP || e->op == EQ_OP || e->op == NE_OP);
  if (optimize && commutative && e->right->label > e->left->label) {
    first = e->right;
    second = e->left;
  }
  first->accept(this);
  code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
  second->accept(this);
  code << "  movq %rax, %rcx\n";
  code << "  movq (%rsp), %rax\n  addq $16, %rsp\n";
  switch (e->op) {
    case ADD_OP: code << "  addq %rcx, %rax\n"; break;
    case SUB_OP: code << "  subq %rcx, %rax\n"; break;
    case MUL_OP: code << "  imulq %rcx, %rax\n"; break;
    case DIV_OP: code << "  cqto\n  idivq %rcx\n"; break;
    case MOD_OP: code << "  cqto\n  idivq %rcx\n  movq %rdx, %rax\n"; break;
    case LT_OP: case GT_OP: case LE_OP: case GE_OP: case EQ_OP: case NE_OP: {
      code << "  cmpq %rcx, %rax\n";
      const char *set = "sete";
      switch (e->op) {
        case LT_OP: set = "setl"; break;
        case GT_OP: set = "setg"; break;
        case LE_OP: set = "setle"; break;
        case GE_OP: set = "setge"; break;
        case EQ_OP: set = "sete"; break;
        case NE_OP: set = "setne"; break;
        default: break;
      }
      code << "  " << set << " %al\n  movzbq %al, %rax\n";
      break;
    }
    default: break;
  }
  return 0;
}

int GenCodeVisitor::visit(UnaryExp *e) {
  if (optimize && e->isConstant) {
    code << "  movq $" << e->constantValue << ", %rax\n";
    return 0;
  }
  e->operand->accept(this);
  if (e->op == NEG_OP) code << "  negq %rax\n";
  else code << "  cmpq $0, %rax\n  sete %al\n  movzbq %al, %rax\n";
  return 0;
}

int GenCodeVisitor::visit(CallExp *e) {
  size_t n = e->args.size();
  if (n > 6)
    throw CompileError("semantico", "el codegen soporta hasta 6 argumentos (metodo '" +
                       e->name + "')", e->line, e->col);
  int nslots = (int)n + ((int)n % 2);
  if (nslots > 0) code << "  subq $" << (8 * nslots) << ", %rsp\n";
  for (size_t k = 0; k < n; k++) {
    e->args[k]->accept(this);
    code << "  movq %rax, " << (8 * (int)k) << "(%rsp)\n";
  }
  for (size_t k = 0; k < n; k++)
    code << "  movq " << (8 * (int)k) << "(%rsp), " << ARG_REGS[k] << "\n";
  if (nslots > 0) code << "  addq $" << (8 * nslots) << ", %rsp\n";
  std::string label = (e->name == "Main") ? "main" : e->name;
  code << "  call " << label << "\n";
  return 0;
}

int GenCodeVisitor::visit(FieldExp *e) {
  int idx = structFieldIndex[varType[e->object]][e->field];
  code << "  movq " << varOffset[e->object] << "(%rbp), %rax\n";
  code << "  movq " << (idx * 8) << "(%rax), %rax\n";
  return 0;
}

int GenCodeVisitor::visit(CastExp *e) {
  if (optimize && e->isConstant) {
    code << "  movq $" << e->constantValue << ", %rax\n";
    return 0;
  }
  e->operand->accept(this);
  // (char): truncar al byte bajo (0..255). (int): el valor ya es de 64 bits.
  if (e->targetType == "char")
    code << "  movzbq %al, %rax\n";
  return 0;
}

int GenCodeVisitor::visit(NewArrayExp *e) {
  e->size->accept(this);
  code << "  movq $8, %rcx\n  imulq %rcx, %rax\n";
  code << "  movq %rax, %rdi\n  call malloc@PLT\n";
  return 0;
}

int GenCodeVisitor::visit(ArrayAccessExp *e) {
  e->index->accept(this);
  code << "  movq %rax, %rcx\n";
  code << "  movq " << varOffset[e->name] << "(%rbp), %rax\n";
  code << "  movq (%rax, %rcx, 8), %rax\n";
  return 0;
}

int GenCodeVisitor::visit(NewMatrixExp *e) {
  e->cols->accept(this);
  code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
  e->rows->accept(this);
  code << "  movq (%rsp), %rcx\n  addq $16, %rsp\n";
  code << "  imulq %rcx, %rax\n";
  code << "  movq $8, %rcx\n  imulq %rcx, %rax\n";
  code << "  movq %rax, %rdi\n  call malloc@PLT\n";
  return 0;
}

void GenCodeVisitor::emitMatrixLinearIndex(MatrixAccessExp *e) {
  long cols = matrixCols.count(e->name) ? matrixCols[e->name] : 0;
  e->rowIdx->accept(this);
  code << "  movq $" << cols << ", %rcx\n  imulq %rcx, %rax\n";
  code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
  e->colIdx->accept(this);
  code << "  movq (%rsp), %rcx\n  addq $16, %rsp\n";
  code << "  addq %rcx, %rax\n";
}

int GenCodeVisitor::visit(MatrixAccessExp *e) {
  emitMatrixLinearIndex(e);
  code << "  movq %rax, %rcx\n";
  code << "  movq " << varOffset[e->name] << "(%rbp), %rax\n";
  code << "  movq (%rax, %rcx, 8), %rax\n";
  return 0;
}

void GenCodeVisitor::emitLoc(int line) {
  if (trackLoc && line > 0 && line != curLine) {
    code << "  # .loc " << line << "\n";
    curLine = line;
  }
}

int GenCodeVisitor::visit(Block *s) {
  auto savedOff = varOffset;
  auto savedTy = varType;
  for (auto st : s->stms) { emitLoc(st->line); st->accept(this); }
  varOffset = savedOff;
  varType = savedTy;
  return 0;
}

int GenCodeVisitor::visit(VarDecStm *s) {
  int off = currentOffset;
  currentOffset -= 8;
  varOffset[s->name] = off;
  varType[s->name] = s->type;
  if (auto nm = dynamic_cast<NewMatrixExp *>(s->init))
    if (auto c = dynamic_cast<NumberExp *>(nm->cols))
      matrixCols[s->name] = c->value;
  if (s->init) {
    s->init->accept(this);
    code << "  movq %rax, " << off << "(%rbp)\n";
  } else if (structFieldIndex.count(s->type)) {
    code << "  movq $" << (structSize[s->type] * 8) << ", %rdi\n";
    code << "  call malloc@PLT\n";
    code << "  movq %rax, " << off << "(%rbp)\n";
  } else {
    code << "  movq $0, " << off << "(%rbp)\n";
  }
  return 0;
}

int GenCodeVisitor::visit(AssignStm *s) {
  if (auto t = dynamic_cast<IdExp *>(s->target)) {
    s->value->accept(this);
    code << "  movq %rax, " << varOffset[t->name] << "(%rbp)\n";
    return 0;
  }
  if (auto f = dynamic_cast<FieldExp *>(s->target)) {
    int idx = structFieldIndex[varType[f->object]][f->field];
    s->value->accept(this);
    code << "  movq %rax, %rcx\n";
    code << "  movq " << varOffset[f->object] << "(%rbp), %rax\n";
    code << "  movq %rcx, " << (idx * 8) << "(%rax)\n";
    return 0;
  }
  if (auto a = dynamic_cast<ArrayAccessExp *>(s->target)) {
    s->value->accept(this);
    code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
    a->index->accept(this);
    code << "  movq %rax, %rcx\n";
    code << "  movq " << varOffset[a->name] << "(%rbp), %rax\n";
    code << "  movq (%rsp), %rdx\n  addq $16, %rsp\n";
    code << "  movq %rdx, (%rax, %rcx, 8)\n";
    return 0;
  }
  if (auto m = dynamic_cast<MatrixAccessExp *>(s->target)) {
    s->value->accept(this);
    code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
    emitMatrixLinearIndex(m);
    code << "  movq %rax, %rcx\n";
    code << "  movq " << varOffset[m->name] << "(%rbp), %rax\n";
    code << "  movq (%rsp), %rdx\n  addq $16, %rsp\n";
    code << "  movq %rdx, (%rax, %rcx, 8)\n";
    return 0;
  }
  return 0;
}

int GenCodeVisitor::visit(ExprStm *s) { s->e->accept(this); return 0; }

int GenCodeVisitor::visit(PrintStm *s) {
  s->e->accept(this);
  std::string t = exprType(s->e);
  const char *fmt = ".Lfmt_int";
  if (t == "string") fmt = ".Lfmt_str";
  else if (t == "char") fmt = ".Lfmt_char";
  code << "  movq %rax, %rsi\n";
  code << "  leaq " << fmt << "(%rip), %rdi\n";
  code << "  xorl %eax, %eax\n";
  code << "  call printf@PLT\n";
  return 0;
}

int GenCodeVisitor::visit(IfStm *s) {
  std::string lelse = newLabel("else");
  std::string lend = newLabel("endif");
  s->cond->accept(this);
  code << "  cmpq $0, %rax\n  je " << (s->elseB ? lelse : lend) << "\n";
  s->thenB->accept(this);
  if (s->elseB) {
    code << "  jmp " << lend << "\n";
    code << lelse << ":\n";
    s->elseB->accept(this);
  }
  code << lend << ":\n";
  return 0;
}

int GenCodeVisitor::visit(WhileStm *s) {
  std::string lstart = newLabel("while");
  std::string lend = newLabel("endwhile");
  std::string savedBreak = currentBreakLabel;
  currentBreakLabel = lend;
  code << lstart << ":\n";
  s->cond->accept(this);
  code << "  cmpq $0, %rax\n  je " << lend << "\n";
  s->body->accept(this);
  code << "  jmp " << lstart << "\n";
  code << lend << ":\n";
  currentBreakLabel = savedBreak;
  return 0;
}

int GenCodeVisitor::visit(DoWhileStm *s) {
  std::string lstart = newLabel("do");
  std::string lend = newLabel("enddo");
  std::string savedBreak = currentBreakLabel;
  currentBreakLabel = lend;
  code << lstart << ":\n";
  s->body->accept(this);
  s->cond->accept(this);
  code << "  cmpq $0, %rax\n  jne " << lstart << "\n";
  code << lend << ":\n";
  currentBreakLabel = savedBreak;
  return 0;
}

int GenCodeVisitor::visit(ForStm *s) {
  auto savedOff = varOffset;
  auto savedTy = varType;
  std::string lstart = newLabel("for");
  std::string lend = newLabel("endfor");
  std::string savedBreak = currentBreakLabel;
  currentBreakLabel = lend;
  if (s->init) s->init->accept(this);
  code << lstart << ":\n";
  if (s->cond) {
    s->cond->accept(this);
    code << "  cmpq $0, %rax\n  je " << lend << "\n";
  }
  s->body->accept(this);
  if (s->update) s->update->accept(this);
  code << "  jmp " << lstart << "\n";
  code << lend << ":\n";
  currentBreakLabel = savedBreak;
  varOffset = savedOff;
  varType = savedTy;
  return 0;
}

int GenCodeVisitor::visit(ReturnStm *s) {
  if (s->e) s->e->accept(this);
  code << "  jmp " << currentRetLabel << "\n";
  return 0;
}

int GenCodeVisitor::visit(BreakStm *) {
  code << "  jmp " << currentBreakLabel << "\n";
  return 0;
}

int GenCodeVisitor::visit(MethodDecl *d) {
  std::string label = (d->name == "Main") ? "main" : d->name;
  varOffset.clear();
  varType.clear();
  currentOffset = -8;
  curLine = -1;
  currentRetLabel = ".L" + label + "_ret";

  int locals = countVars(d->body);
  int nparams = (int)d->params.size();
  int frame = align16((locals + nparams) * 8);

  if (label == "main") code << ".globl main\n";
  code << label << ":\n";
  code << "  pushq %rbp\n  movq %rsp, %rbp\n";
  if (frame > 0) code << "  subq $" << frame << ", %rsp\n";

  for (int i = 0; i < nparams && i < 6; i++) {
    int off = currentOffset;
    currentOffset -= 8;
    varOffset[d->params[i].name] = off;
    varType[d->params[i].name] = d->params[i].type;
    code << "  movq " << ARG_REGS[i] << ", " << off << "(%rbp)\n";
  }

  d->body->accept(this);
  code << "  movq $0, %rax\n";
  code << currentRetLabel << ":\n";
  code << "  leave\n  ret\n";
  return 0;
}

int GenCodeVisitor::visit(ClassDecl *d) {
  for (auto m : d->methods) {
    m->accept(this);
    code << "\n";
  }
  return 0;
}

int GenCodeVisitor::visit(StructDecl *sd) {
  for (size_t i = 0; i < sd->fields.size(); i++)
    structFieldIndex[sd->name][sd->fields[i].name] = (int)i;
  structSize[sd->name] = (int)sd->fields.size();
  return 0;
}

int GenCodeVisitor::visit(Program *p) {
  for (auto *s : p->structs) s->accept(this);
  for (auto c : p->classes) c->accept(this);
  return 0;
}

void GenCodeVisitor::generar(Program *p) {
  collectStrings(p);
  p->accept(this);
}

std::string GenCodeVisitor::render() {
  std::ostringstream out;
  out << ".data\n";
  out << ".Lfmt_int:  .string \"%ld\\n\"\n";
  out << ".Lfmt_char: .string \"%c\\n\"\n";
  out << ".Lfmt_str:  .string \"%s\\n\"\n";
  for (size_t i = 0; i < stringLits.size(); i++)
    out << ".Lstr" << i << ": .string \"" << escapeForAsm(stringLits[i]) << "\"\n";
  out << "\n.text\n";
  out << code.str();
  out << ".section .note.GNU-stack,\"\",@progbits\n";
  return out.str();
}

void GenCodeVisitor::write(std::ostream &out) { out << render(); }
