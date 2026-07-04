#include "visitor.h"
#include "ast.h"
#include "errors.h"
#include <cstdint>

namespace {
const char *ARG_REGS[6] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
const char *ARG_REGS32[6] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};

int align16(int n) { return (n + 15) & ~15; }
int alignTo(int n, int a) { return (n + a - 1) & ~(a - 1); }

std::string stackAddr(int off) { return std::to_string(off) + "(%rbp)"; }
std::string ptrAddr(int off, const std::string &base = "%rax") {
  return std::to_string(off) + "(" + base + ")";
}
std::string indexedAddr(const std::string &base, const std::string &index, int scale) {
  return "(" + base + "," + index + "," + std::to_string(scale) + ")";
}

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

bool GenCodeVisitor::isPrimitive32(const std::string &type) const {
  return type == "int" || type == "char" || type == "bool";
}

int GenCodeVisitor::storageSize(const std::string &type) const {
  return isPrimitive32(type) ? 4 : 8;
}

int GenCodeVisitor::storageAlign(const std::string &type) const {
  int size = storageSize(type);
  return size < 8 ? size : 8;
}

int GenCodeVisitor::allocateSlot(const std::string &type) {
  int used = -currentOffset;
  used = alignTo(used, storageAlign(type));
  used += storageSize(type);
  currentOffset = -used;
  return currentOffset;
}

void GenCodeVisitor::emitIntConst(long value) {
  code << "  movl $" << static_cast<int32_t>(value) << ", %eax\n";
}

void GenCodeVisitor::emitLoad(const std::string &type, const std::string &addr) {
  if (isPrimitive32(type)) code << "  movslq " << addr << ", %rax\n";
  else code << "  movq " << addr << ", %rax\n";
}

void GenCodeVisitor::emitStore(const std::string &type, const std::string &addr,
                               const std::string &reg64, const std::string &reg32) {
  if (isPrimitive32(type)) code << "  movl " << reg32 << ", " << addr << "\n";
  else code << "  movq " << reg64 << ", " << addr << "\n";
}

void GenCodeVisitor::emitZeroStore(const std::string &type, const std::string &addr) {
  if (isPrimitive32(type)) code << "  movl $0, " << addr << "\n";
  else code << "  movq $0, " << addr << "\n";
}

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
  else if (auto us = dynamic_cast<UnsafeStm *>(s)) collectStm(us->body, lits);
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
  } else if (auto de = dynamic_cast<DerefExp *>(e)) {
    collectExp(de->ptr, lits);
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
  if (auto us = dynamic_cast<UnsafeStm *>(s)) return countVars(us->body);
  return 0;
}

int GenCodeVisitor::visit(NumberExp *e) { emitIntConst(e->value); return 0; }
int GenCodeVisitor::visit(BoolExp *e) { emitIntConst(e->value ? 1 : 0); return 0; }
int GenCodeVisitor::visit(CharExp *e) { emitIntConst(e->value); return 0; }
int GenCodeVisitor::visit(StringExp *e) { code << "  leaq .Lstr" << e->labelId << "(%rip), %rax\n"; return 0; }
int GenCodeVisitor::visit(IdExp *e) {
  emitLoad(varType[e->name], stackAddr(varOffset[e->name]));
  return 0;
}

int GenCodeVisitor::visit(BinaryExp *e) {
  if (optimize && e->isConstant) {
    emitIntConst(e->constantValue);
    return 0;
  }
  if (e->op == AND_OP || e->op == OR_OP) {
    std::string lend = newLabel("bool_end");
    std::string lset = newLabel("bool_set");
    e->left->accept(this);
    code << "  cmpl $0, %eax\n";
    if (e->op == AND_OP) {
      code << "  je " << lset << "\n";
      e->right->accept(this);
      code << "  cmpl $0, %eax\n  je " << lset << "\n";
      code << "  movl $1, %eax\n  jmp " << lend << "\n";
      code << lset << ":\n  movl $0, %eax\n";
    } else {
      code << "  jne " << lset << "\n";
      e->right->accept(this);
      code << "  cmpl $0, %eax\n  jne " << lset << "\n";
      code << "  movl $0, %eax\n  jmp " << lend << "\n";
      code << lset << ":\n  movl $1, %eax\n";
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
    case ADD_OP: code << "  addl %ecx, %eax\n"; break;
    case SUB_OP: code << "  subl %ecx, %eax\n"; break;
    case MUL_OP: code << "  imull %ecx, %eax\n"; break;
    case DIV_OP: code << "  cltd\n  idivl %ecx\n"; break;
    case MOD_OP: code << "  cltd\n  idivl %ecx\n  movl %edx, %eax\n"; break;
    case LT_OP: case GT_OP: case LE_OP: case GE_OP: case EQ_OP: case NE_OP: {
      bool cmp32 = isPrimitive32(exprType(e->left)) && isPrimitive32(exprType(e->right));
      code << "  " << (cmp32 ? "cmpl %ecx, %eax" : "cmpq %rcx, %rax") << "\n";
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
      code << "  " << set << " %al\n  movzbl %al, %eax\n";
      break;
    }
    default: break;
  }
  return 0;
}

int GenCodeVisitor::visit(UnaryExp *e) {
  if (optimize && e->isConstant) {
    emitIntConst(e->constantValue);
    return 0;
  }
  e->operand->accept(this);
  if (e->op == NEG_OP) code << "  negl %eax\n";
  else code << "  cmpl $0, %eax\n  sete %al\n  movzbl %al, %eax\n";
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
  const std::string &objType = varType[e->object];
  int off = structFieldOffset[objType][e->field];
  emitLoad(objType, stackAddr(varOffset[e->object]));
  emitLoad(structFieldType[objType][e->field], ptrAddr(off));
  return 0;
}

int GenCodeVisitor::visit(CastExp *e) {
  if (optimize && e->isConstant) {
    emitIntConst(e->constantValue);
    return 0;
  }
  e->operand->accept(this);
  if (e->targetType == "char")
    code << "  movzbl %al, %eax\n";
  return 0;
}

int GenCodeVisitor::visit(AddressOfExp *e) {
  code << "  leaq " << varOffset[e->name] << "(%rbp), %rax\n";
  return 0;
}

int GenCodeVisitor::visit(DerefExp *e) {
  e->ptr->accept(this);
  emitLoad(exprType(e), "(%rax)");
  return 0;
}

int GenCodeVisitor::visit(NewArrayExp *e) {
  e->size->accept(this);
  code << "  movslq %eax, %rax\n";
  code << "  imulq $" << storageSize(e->elemType) << ", %rax, %rax\n";
  code << "  movq %rax, %rdi\n  call malloc@PLT\n";
  return 0;
}

int GenCodeVisitor::visit(ArrayAccessExp *e) {
  e->index->accept(this);
  code << "  movslq %eax, %rcx\n";
  emitLoad(varType[e->name], stackAddr(varOffset[e->name]));
  emitLoad(exprType(e), indexedAddr("%rax", "%rcx", storageSize(exprType(e))));
  return 0;
}

int GenCodeVisitor::visit(NewMatrixExp *e) {
  e->cols->accept(this);
  code << "  movslq %eax, %rax\n";
  code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
  e->rows->accept(this);
  code << "  movslq %eax, %rax\n";
  code << "  movq (%rsp), %rcx\n  addq $16, %rsp\n";
  code << "  imulq %rcx, %rax\n";
  code << "  imulq $" << storageSize(e->elemType) << ", %rax, %rax\n";
  code << "  movq %rax, %rdi\n  call malloc@PLT\n";
  return 0;
}

void GenCodeVisitor::emitMatrixLinearIndex(MatrixAccessExp *e) {
  long cols = matrixCols.count(e->name) ? matrixCols[e->name] : 0;
  e->rowIdx->accept(this);
  code << "  movslq %eax, %rax\n";
  code << "  movq $" << cols << ", %rcx\n  imulq %rcx, %rax\n";
  code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
  e->colIdx->accept(this);
  code << "  movslq %eax, %rax\n";
  code << "  movq (%rsp), %rcx\n  addq $16, %rsp\n";
  code << "  addq %rcx, %rax\n";
}

int GenCodeVisitor::visit(MatrixAccessExp *e) {
  emitMatrixLinearIndex(e);
  code << "  movq %rax, %rcx\n";
  emitLoad(varType[e->name], stackAddr(varOffset[e->name]));
  emitLoad(exprType(e), indexedAddr("%rax", "%rcx", storageSize(exprType(e))));
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
  int off = allocateSlot(s->type);
  varOffset[s->name] = off;
  varType[s->name] = s->type;
  if (auto nm = dynamic_cast<NewMatrixExp *>(s->init))
    if (auto c = dynamic_cast<NumberExp *>(nm->cols))
      matrixCols[s->name] = c->value;
  if (s->init) {
    s->init->accept(this);
    emitStore(s->type, stackAddr(off));
  } else if (structFieldIndex.count(s->type)) {
    code << "  movq $" << structSizeBytes[s->type] << ", %rdi\n";
    code << "  call malloc@PLT\n";
    emitStore(s->type, stackAddr(off));
  } else {
    emitZeroStore(s->type, stackAddr(off));
  }
  return 0;
}

int GenCodeVisitor::visit(AssignStm *s) {
  if (auto t = dynamic_cast<IdExp *>(s->target)) {
    s->value->accept(this);
    emitStore(varType[t->name], stackAddr(varOffset[t->name]));
    return 0;
  }
  if (auto f = dynamic_cast<FieldExp *>(s->target)) {
    const std::string &objType = varType[f->object];
    int off = structFieldOffset[objType][f->field];
    const std::string &fieldType = structFieldType[objType][f->field];
    s->value->accept(this);
    code << "  movq %rax, %rcx\n";
    emitLoad(objType, stackAddr(varOffset[f->object]));
    emitStore(fieldType, ptrAddr(off), "%rcx", "%ecx");
    return 0;
  }
  if (auto a = dynamic_cast<ArrayAccessExp *>(s->target)) {
    s->value->accept(this);
    code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
    a->index->accept(this);
    code << "  movslq %eax, %rcx\n";
    emitLoad(varType[a->name], stackAddr(varOffset[a->name]));
    code << "  movq (%rsp), %rdx\n  addq $16, %rsp\n";
    emitStore(exprType(a), indexedAddr("%rax", "%rcx", storageSize(exprType(a))), "%rdx", "%edx");
    return 0;
  }
  if (auto m = dynamic_cast<MatrixAccessExp *>(s->target)) {
    s->value->accept(this);
    code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
    emitMatrixLinearIndex(m);
    code << "  movq %rax, %rcx\n";
    emitLoad(varType[m->name], stackAddr(varOffset[m->name]));
    code << "  movq (%rsp), %rdx\n  addq $16, %rsp\n";
    emitStore(exprType(m), indexedAddr("%rax", "%rcx", storageSize(exprType(m))), "%rdx", "%edx");
    return 0;
  }
  if (auto de = dynamic_cast<DerefExp *>(s->target)) {
    s->value->accept(this);
    code << "  subq $16, %rsp\n  movq %rax, (%rsp)\n";
    de->ptr->accept(this);
    code << "  movq (%rsp), %rcx\n  addq $16, %rsp\n";
    emitStore(exprType(de), "(%rax)", "%rcx", "%ecx");
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
  if (t == "string") code << "  movq %rax, %rsi\n";
  else code << "  movl %eax, %esi\n";
  code << "  leaq " << fmt << "(%rip), %rdi\n";
  code << "  xorl %eax, %eax\n";
  code << "  call printf@PLT\n";
  return 0;
}

int GenCodeVisitor::visit(IfStm *s) {
  std::string lelse = newLabel("else");
  std::string lend = newLabel("endif");
  s->cond->accept(this);
  code << "  cmpl $0, %eax\n  je " << (s->elseB ? lelse : lend) << "\n";
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
  code << "  cmpl $0, %eax\n  je " << lend << "\n";
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
  code << "  cmpl $0, %eax\n  jne " << lstart << "\n";
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
    code << "  cmpl $0, %eax\n  je " << lend << "\n";
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

int GenCodeVisitor::visit(UnsafeStm *s) {
  s->body->accept(this);
  return 0;
}

int GenCodeVisitor::visit(MethodDecl *d) {
  std::string label = (d->name == "Main") ? "main" : d->name;
  varOffset.clear();
  varType.clear();
  currentOffset = 0;
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
    int off = allocateSlot(d->params[i].type);
    varOffset[d->params[i].name] = off;
    varType[d->params[i].name] = d->params[i].type;
    emitStore(d->params[i].type, stackAddr(off), ARG_REGS[i], ARG_REGS32[i]);
  }

  d->body->accept(this);
  code << "  movl $0, %eax\n";
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
  int offset = 0;
  int maxAlign = 1;
  for (size_t i = 0; i < sd->fields.size(); i++) {
    int align = storageAlign(sd->fields[i].type);
    offset = alignTo(offset, align);
    structFieldIndex[sd->name][sd->fields[i].name] = (int)i;
    structFieldOffset[sd->name][sd->fields[i].name] = offset;
    structFieldType[sd->name][sd->fields[i].name] = sd->fields[i].type;
    offset += storageSize(sd->fields[i].type);
    if (align > maxAlign) maxAlign = align;
  }
  structSizeBytes[sd->name] = alignTo(offset, maxAlign);
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
  out << ".Lfmt_int:  .string \"%d\\n\"\n";
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
