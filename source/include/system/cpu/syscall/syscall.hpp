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

char *read(char *string, int length);
char *write(char *string, int length);
char *err(char *string, int length);

extern "C" void syscall_handler(idt::interrupt_registers *regs);
}