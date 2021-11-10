%macro __pusha 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro __popa 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

[EXTERN int_handler]
int_common_stub:
    __pusha
    mov rdi, rsp
    call int_handler
    __popa
    add rsp, 16
    iretq

%macro isr_stub 1
isr_stub_%+%1:
    push byte 0
    push byte %1
    jmp int_common_stub
%endmacro

%macro isr_errcode_stub 1
isr_stub_%+%1:
    push byte %1
    jmp int_common_stub
%endmacro

%macro irq_stub 1
irq_stub_%+%1:
    push byte %1
    push byte 32 + %1
    jmp int_common_stub
%endmacro

; ISRs
isr_stub 0
isr_stub 1
isr_stub 2
isr_stub 3
isr_stub 4
isr_stub 5
isr_stub 6
isr_stub 7
isr_errcode_stub 8
isr_stub 9
isr_errcode_stub 10
isr_errcode_stub 11
isr_errcode_stub 12
isr_errcode_stub 13
isr_errcode_stub 14
isr_stub 15
isr_stub 16
isr_stub 17
isr_stub 18
isr_stub 19
isr_stub 20
isr_stub 21
isr_stub 22
isr_stub 23
isr_stub 24
isr_stub 25
isr_stub 26
isr_stub 27
isr_stub 28
isr_stub 29
isr_stub 30
isr_stub 31

; IRQs
irq_stub 0
irq_stub 1
irq_stub 2
irq_stub 3
irq_stub 4
irq_stub 5
irq_stub 6
irq_stub 7
irq_stub 8
irq_stub 9
irq_stub 10
irq_stub 11
irq_stub 12
irq_stub 13
irq_stub 14
irq_stub 15

syscall:
    push byte 0
    push dword 0x80
    jmp int_common_stub

section .data

int_table:
%assign i 0
%rep 32
    dq isr_stub_%+i
%assign i i+1
%endrep

%assign i 0
%rep 16
    dq irq_stub_%+i
%assign i i+1
%endrep

%rep 80
    dq 0
%endrep
    dq syscall
GLOBAL int_table