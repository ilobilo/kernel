#pragma once

#include <system/cpu/idt/idt.hpp>
#include <stdint.h>

#define S_RAX regs->rax
#define S_ARG0_RDI regs->rdi
#define S_ARG1_RSI regs->rsi
#define S_ARG2_RDX regs->rdx
#define S_ARG3_R10 regs->r10
#define S_ARG4_R8 regs->r8
#define S_ARG5_R9 regs->r9

using syscall_t = void (*)(interrupt_registers *);

char *s_read(char *string, int length);
void s_write(char *string, int length);
void s_err(char *string, int length);

extern "C" void syscall_handler(interrupt_registers *regs);