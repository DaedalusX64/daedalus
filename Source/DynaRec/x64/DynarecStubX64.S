.globl __EnterDynaRec
.text
__EnterDynaRec:
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

    ret