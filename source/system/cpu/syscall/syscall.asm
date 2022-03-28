%include "lib/cpu.inc"

EXTERN syscall_table

syscall_entry:
    swapgs
    mov gs:[8], rsp
    mov rsp, gs:[16]
    sti

    push qword 0x43
    push qword gs:[8]
    push r11
    push qword 0x3B
    push rcx
    push 0
    push 0
    pushall

    mov rdi, rsp
    lea rbx, [rel syscall_table]
    call [rbx + rax * 8]

    popall
    add rsp, 56

    cli
    swapgs
    o64 sysret
GLOBAL syscall_entry