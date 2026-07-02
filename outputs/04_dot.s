.data
.Lfmt_int:  .string "%ld\n"
.Lfmt_char: .string "%c\n"
.Lfmt_str:  .string "%s\n"

.text
Area:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movq %rdi, -8(%rbp)
  movq -8(%rbp), %rax
  movq 0(%rax), %rax
  subq $16, %rsp
  movq %rax, (%rsp)
  movq -8(%rbp), %rax
  movq 8(%rax), %rax
  movq %rax, %rcx
  movq (%rsp), %rax
  addq $16, %rsp
  imulq %rcx, %rax
.LArea_ret:
  leave
  ret
.globl main
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $16, %rsp
  movq $16, %rdi
  call malloc@PLT
  movq %rax, -8(%rbp)
  movq $6, %rax
  movq %rax, %rcx
  movq -8(%rbp), %rax
  movq %rcx, 0(%rax)
  movq $7, %rax
  movq %rax, %rcx
  movq -8(%rbp), %rax
  movq %rcx, 8(%rax)
  movq -8(%rbp), %rax
  movq 0(%rax), %rax
  movq %rax, %rsi
  leaq .Lfmt_int(%rip), %rdi
  xorl %eax, %eax
  call printf@PLT
  movq -8(%rbp), %rax
  movq 8(%rax), %rax
  movq %rax, %rsi
  leaq .Lfmt_int(%rip), %rdi
  xorl %eax, %eax
  call printf@PLT
  subq $16, %rsp
  movq -8(%rbp), %rax
  movq %rax, 0(%rsp)
  movq 0(%rsp), %rdi
  addq $16, %rsp
  call Area
  movq %rax, %rsi
  leaq .Lfmt_int(%rip), %rdi
  xorl %eax, %eax
  call printf@PLT
  movq $0, %rax
.Lmain_ret:
  leave
  ret
.section .note.GNU-stack,"",@progbits
