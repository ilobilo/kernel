// Copyright (C) 2021  ilobilo

#pragma once

#include <system/cpu/idt/idt.hpp>
#include <stdint.h>

namespace kernel::system::cpu::syscall {

#define S_RAX regs->rax
#define S_RDI_ARG0 regs->rdi
#define S_RSI_ARG1 regs->rsi
#define S_RDX_ARG2 regs->rdx
#define S_R10_ARG3 regs->r10
#define S_R8_ARG4 regs->r8
#define S_R9_ARG5 regs->r9

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

#define SYSCALL4(NUM, ARG0, ARG1, ARG2, ARG3) ({ \
    register typeof(ARG3) arg3 asm("r10") = ARG3; \
    asm volatile ("int $0x80" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1), "d"(ARG2), "r"(arg3) : "rcx", "r11", "memory"); \
})

#define SYSCALL5(NUM, ARG0, ARG1, ARG2, ARG3, ARG4) ({ \
    register typeof(ARG3) arg3 asm("r10") = ARG3; \
    register typeof(ARG4) arg4 asm("r8")  = ARG4; \
    asm volatile ("int $0x80" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1), "d"(ARG2), "r"(arg3), "r"(arg4), "r"(arg5) : "rcx", "r11", "memory"); \
})

#define SYSCALL6(NUM, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) ({ \
    register typeof(ARG3) arg3 asm("r10") = ARG3; \                                                                                                    +    register typeof(ARG4) arg4 asm("r8")  = ARG4; \
    register typeof(ARG5) arg5 asm("r9")  = ARG5; \
    asm volatile ("int $0x80" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1), "d"(ARG2), "r"(arg3), "r"(arg4), "r"(arg5) : "rcx", "r11", "memory"); \
})

#define SYSCALL_READ	0
#define SYSCALL_WRITE	1

using syscall_t = void (*)(idt::interrupt_registers *);

const char *read(const char *string, int length);
const char *write(const char *string, int length);
const char *err(const char *string, int length);

void handler(idt::interrupt_registers *regs);
}