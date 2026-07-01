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
  movq $14, %rax
  movq %rax, -8(%rbp)
  movq -8(%rbp), %rax
  movq %rax, %rsi
  leaq .Lfmt_int(%rip), %rdi
  xorl %eax, %eax
  call printf@PLT
  movq $0, %rax
.Lmain_ret:
  leave
  ret
.section .note.GNU-stack,"",@progbits
