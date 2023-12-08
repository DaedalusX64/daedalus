.code

PUBLIC _EnterDynaRec

_EnterDynaRec PROC
    push rbp
    push rbx
    push r15
    push r14


    mov rax, rcx
    mov rbx, rdx
    mov r15, r8
    mov r14, r9

    jmp rax

_EnterDynaRec ENDP

END
