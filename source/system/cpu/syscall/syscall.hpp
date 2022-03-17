// Copyright (C) 2021-2022  ilobilo

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

static constexpr uint64_t SYSCALL_READ = 0;
static constexpr uint64_t SYSCALL_WRITE = 1;
static constexpr uint64_t SYSCALL_OPEN = 2;
static constexpr uint64_t SYSCALL_CLOSE = 3;
static constexpr uint64_t SYSCALL_IOCTL = 16;
static constexpr uint64_t SYSCALL_GETPID = 39;
static constexpr uint64_t SYSCALL_OPENAT = 56;
static constexpr uint64_t SYSCALL_EXIT = 60;
static constexpr uint64_t SYSCALL_UNAME = 63;
static constexpr uint64_t SYSCALL_CHDIR = 80;
static constexpr uint64_t SYSCALL_MKDIR = 83;
static constexpr uint64_t SYSCALL_RMDIR = 84;
static constexpr uint64_t SYSCALL_CHMOD = 90;
static constexpr uint64_t SYSCALL_FCHMOD = 91;
static constexpr uint64_t SYSCALL_CHOWN = 92;
static constexpr uint64_t SYSCALL_FCHOWN = 93;
static constexpr uint64_t SYSCALL_LCHOWN = 94;
static constexpr uint64_t SYSCALL_SYSINFO = 99;
static constexpr uint64_t SYSCALL_GETPPID = 110;
static constexpr uint64_t SYSCALL_REBOOT = 169;
static constexpr uint64_t SYSCALL_TIME = 201;
static constexpr uint64_t SYSCALL_MKDIRAT = 258;
static constexpr uint64_t SYSCALL_UNLINKAT = 263;
static constexpr uint64_t SYSCALL_READLINKAT = 267;

using syscall_t = void (*)(registers_t *);

extern bool initialised;

const char *read(const char *string, int length);
const char *write(const char *string, int length);
const char *err(const char *string, int length);
void reboot(const char *message);

void init();
}