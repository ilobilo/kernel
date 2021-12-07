%include "lib/cpu.inc"

Switch:
    pushall
    mov [rdi], rsp
    mov rsp, rsi
    popall
    ret
GLOBAL Switch