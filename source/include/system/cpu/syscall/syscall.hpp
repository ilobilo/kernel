#pragma once

#include <system/cpu/idt/idt.hpp>
#include <stdint.h>

namespace kernel::system::cpu::syscall {

#define S_RAX regs->rax
#define S_ARG0_RDI regs->rdi
#define S_ARG1_RSI regs->rsi
#define S_ARG2_RDX regs->rdx
#define S_ARG3_R10 regs->r10
#define S_ARG4_R8 regs->r8
#define S_ARG5_R9 regs->r9

#define SYSCALL0(NUM) \
({ \
    asm volatile ("int $0x80" : "=a"(ret) : "a"(NUM) : "rcx", "r11", "memory"); \
})

#define SYSCALL1(NUM, ARG0) \
({ \
    asm volatile ("int $0x80" : "=a"(ret) : "a"(NUM), "D"(ARG0) : "rcx", "r11", "memory"); \
})

#define SYSCALL2(NUM, ARG0, ARG1) \
({ \
    asm volatile ("int $0x80" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1) : "rcx", "r11", "memory"); \
})

#define SYSCALL3(NUM, ARG0, ARG1, ARG2) \
({ \
    asm volatile ("int $0x80" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1), "d"(ARG2) : "rcx", "r11", "memory"); \
})

using syscall_t = void (*)(idt::interrupt_registers *);

char *read(char *string, int length);
void write(char *string, int length);
void err(char *string, int length);

extern "C" void syscall_handler(idt::interrupt_registers *regs);
}