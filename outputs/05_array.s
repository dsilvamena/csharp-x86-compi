.data
.Lfmt_int:  .string "%ld\n"
.Lfmt_char: .string "%c\n"
.Lfmt_str:  .string "%s\n"

.text
.globl main
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movq $3, %rax
  movq $8, %rcx
  imulq %rcx, %rax
  movq %rax, %rdi
  call malloc@PLT
  movq %rax, -8(%rbp)
  movq $4, %rax
  subq $16, %rsp
  movq %rax, (%rsp)
  movq $0, %rax
  movq %rax, %rcx
  movq -8(%rbp), %rax
  movq (%rsp), %rdx
  addq $16, %rsp
  movq %rdx, (%rax, %rcx, 8)
  movq $5, %rax
  subq $16, %rsp
  movq %rax, (%rsp)
  movq $1, %rax
  movq %rax, %rcx
  movq -8(%rbp), %rax
  movq (%rsp), %rdx
  addq $16, %rsp
  movq %rdx, (%rax, %rcx, 8)
  movq $0, %rax
  movq %rax, %rcx
  movq -8(%rbp), %rax
  movq (%rax, %rcx, 8), %rax
  subq $16, %rsp
  movq %rax, (%rsp)
  movq $1, %rax
  movq %rax, %rcx
  movq -8(%rbp), %rax
  movq (%rax, %rcx, 8), %rax
  movq %rax, %rcx
  movq (%rsp), %rax
  addq $16, %rsp
  addq %rcx, %rax
  subq $16, %rsp
  movq %rax, (%rsp)
  movq $2, %rax
  movq %rax, %rcx
  movq -8(%rbp), %rax
  movq (%rsp), %rdx
  addq $16, %rsp
  movq %rdx, (%rax, %rcx, 8)
  movq $2, %rax
  movq %rax, %rcx
  movq -8(%rbp), %rax
  movq (%rax, %rcx, 8), %rax
  movq %rax, %rsi
  leaq .Lfmt_int(%rip), %rdi
  xorl %eax, %eax
  call printf@PLT
  movq $0, %rax
.Lmain_ret:
  leave
  ret
.section .note.GNU-stack,"",@progbits
