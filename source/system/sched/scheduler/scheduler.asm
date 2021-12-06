%include "lib/cpu.inc"

context_switch:
    pushall
    mov [rdi], rsp
    mov rsp, rsi
    popall
    ret
GLOBAL context_switch