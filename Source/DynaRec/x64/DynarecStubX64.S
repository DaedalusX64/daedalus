.globl __EnterDynaRec
.globl _EnterDynaRec
.text
_EnterDynaRec:
__EnterDynaRec:
    pushq %rbp
    pushq %rbx
    pushq %r15
    pushq %r14

    movq %rdi, %rax
    movq %rsi, %rbx
    movq %rdx, %r15
    movq %rcx, %r14

    call *%rax

    popq %r14
    popq %r15
    popq %rbx
    popq %rbp

    ret