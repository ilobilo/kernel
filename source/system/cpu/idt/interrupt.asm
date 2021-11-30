%macro pushall 0
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

[EXTERN int_handler]
int_common_stub:
    pushall
    mov rdi, rsp
    call int_handler
    popall
    add rsp, 24
    iretq

%macro isr 1
isr_%1:
    push 0
    push %1
    push fs
    jmp int_common_stub
%endmacro

%macro isr_err 1
isr_%1:
    push %1
    push fs
    jmp int_common_stub
%endmacro

; Exceptions
isr 0
isr 1
isr 2
isr 3
isr 4
isr 5
isr 6
isr 7
isr_err 8
isr 9
isr_err 10
isr_err 11
isr_err 12
isr_err 13
isr_err 14
isr 15
isr 16
isr 17
isr 18
isr 19
isr 20
isr 21
isr 22
isr 23
isr 24
isr 25
isr 26
isr 27
isr 28
isr 29
isr 30
isr 31

; IRQs and other
%assign i 32
%rep 225
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