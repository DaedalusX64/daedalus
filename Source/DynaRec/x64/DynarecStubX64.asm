.code

PUBLIC _EnterDynaRec

_EnterDynaRec PROC FRAME
    push rbp
.PUSHREG RBP
    push rbx
.PUSHREG RBX
    push r15
.PUSHREG R15
    push r14
.PUSHREG R14
    sub rsp, 32
.allocstack 32
.ENDPROLOG
    mov rax, rcx
    mov rbx, rdx
    mov r15, r8
    mov r14, r9

    call rax

    add rsp, 32
    pop r14
    pop r15
    pop rbx
    pop rbp

    ret
_EnterDynaRec ENDP

END
