.code

PUBLIC _EnterDynaRec

_EnterDynaRec PROC
    push rbp
    push rbx
    push r15
    push r14


    mov rax, rcx
    mov rcx, rdx
    mov r15, r8
    mov r14, r9

    call qword ptr [rax]

    pop r14
    pop r15
    pop rbx
    pop rbp

    ret
_EnterDynaRec ENDP

END