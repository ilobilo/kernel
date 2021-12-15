%include "lib/cpu.inc"

[EXTERN int_handler]
int_common_stub:
    pushall
    mov rdi, rsp
    call int_handler
    popall
    add rsp, 16
    iretq

%macro isr 1
isr_%1:
%if !(%1 == 8 || (%1 >= 10 && %1 <= 14) || %1 == 17 || %1 == 21 || %1 == 29 || %1 == 30)
    push 0
%endif
    push %1
    jmp int_common_stub
%endmacro

%assign i 0
%rep 256
isr i
%assign i i+1
%endrep

; Interrupts array
section .data
int_table:
%assign i 0
%rep 256
    dq isr_%+i
%assign i i+1
%endrep
GLOBAL int_table