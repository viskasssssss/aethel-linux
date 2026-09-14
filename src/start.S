.global _start

.section .text

_start:
    call main

    mov %rax, %rdi
    mov $60, %rax
    syscall