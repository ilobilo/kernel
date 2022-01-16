// Copyright (C) 2021  ilobilo

#pragma once

#include <system/cpu/idt/idt.hpp>
#include <stdint.h>

namespace kernel::system::cpu::syscall {

#define RAX regs->rax
#define RDI_ARG0 regs->rdi
#define RSI_ARG1 regs->rsi
#define RDX_ARG2 regs->rdx
#define R10_ARG3 regs->r10
#define R8_ARG4 regs->r8
#define R9_ARG5 regs->r9

#define SYSCALL0(NUM) \
({ \
    asm volatile ("int $0x69" : "=a"(ret) : "a"(NUM) : "rcx", "r11", "memory"); \
})

#define SYSCALL1(NUM, ARG0) \
({ \
    asm volatile ("int $0x69" : "=a"(ret) : "a"(NUM), "D"(ARG0) : "rcx", "r11", "memory"); \
})

#define SYSCALL2(NUM, ARG0, ARG1) \
({ \
    asm volatile ("int $0x69" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1) : "rcx", "r11", "memory"); \
})

#define SYSCALL3(NUM, ARG0, ARG1, ARG2) \
({ \
    asm volatile ("int $0x69" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1), "d"(ARG2) : "rcx", "r11", "memory"); \
})

#define SYSCALL4(NUM, ARG0, ARG1, ARG2, ARG3) ({ \
    register typeof(ARG3) arg3 asm("r10") = ARG3; \
    asm volatile ("int $0x69" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1), "d"(ARG2), "r"(arg3) : "rcx", "r11", "memory"); \
})

#define SYSCALL5(NUM, ARG0, ARG1, ARG2, ARG3, ARG4) ({ \
    register typeof(ARG3) arg3 asm("r10") = ARG3; \
    register typeof(ARG4) arg4 asm("r8")  = ARG4; \
    asm volatile ("int $0x69" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1), "d"(ARG2), "r"(arg3), "r"(arg4), "r"(arg5) : "rcx", "r11", "memory"); \
})

#define SYSCALL6(NUM, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) ({ \
    register typeof(ARG3) arg3 asm("r10") = ARG3; \                                                                                                    +    register typeof(ARG4) arg4 asm("r8")  = ARG4; \
    register typeof(ARG5) arg5 asm("r9")  = ARG5; \
    asm volatile ("int $0x69" : "=a"(ret) : "a"(NUM), "D"(ARG0), "S"(ARG1), "d"(ARG2), "r"(arg3), "r"(arg4), "r"(arg5) : "rcx", "r11", "memory"); \
})

#define SYSCALL_READ 0
#define SYSCALL_WRITE 1
#define SYSCALL_GETPID 39
#define SYSCALL_EXIT 60
#define SYSCALL_UNAME 63
#define SYSCALL_CHDIR 80
#define SYSCALL_RENAME 82
#define SYSCALL_MKDIR 83
#define SYSCALL_RMDIR 84
#define SYSCALL_CHMOD 90
#define SYSCALL_CHOWN 92
#define SYSCALL_SYSINFO 99
#define SYSCALL_REBOOT 169
#define SYSCALL_TIME 201

using syscall_t = void (*)(registers_t *);

extern bool initialised;

const char *read(const char *string, int length);
const char *write(const char *string, int length);
const char *err(const char *string, int length);

void init();
}