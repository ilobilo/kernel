// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/cpu/idt/idt.hpp>
#include <lib/string.hpp>
#include <cstdint>

namespace kernel::system::cpu::syscall {

struct syscall_ret
{
    uint64_t retval;
    uint64_t err;
};

#define RDX_ERRNO regs->rdx
#define RAX_RET regs->rax
#define RDI_ARG0 regs->rdi
#define RSI_ARG1 regs->rsi
#define RDX_ARG2 regs->rdx
#define R10_ARG3 regs->r10
#define R8_ARG4 regs->r8
#define R9_ARG5 regs->r9

enum syscalls
{
    SYSCALL_READ = 0,
    SYSCALL_WRITE = 1,
    SYSCALL_OPEN = 2,
    SYSCALL_CLOSE = 3,
    SYSCALL_IOCTL = 16,
    SYSCALL_ACCESS = 21,
    SYSCALL_GETPID = 39,
    SYSCALL_FORK = 57,
    SYSCALL_EXIT = 60,
    SYSCALL_UNAME = 63,
    SYSCALL_GETCWD = 79,
    SYSCALL_CHDIR = 80,
    SYSCALL_MKDIR = 83,
    SYSCALL_RMDIR = 84,
    SYSCALL_LINK = 86,
    SYSCALL_CHMOD = 90,
    SYSCALL_FCHMOD = 91,
    SYSCALL_CHOWN = 92,
    SYSCALL_FCHOWN = 93,
    SYSCALL_LCHOWN = 94,
    SYSCALL_SYSINFO = 99,
    SYSCALL_GETPPID = 110,
    SYSCALL_MOUNT = 165,
    SYSCALL_REBOOT = 169,
    SYSCALL_TIME = 201,
    SYSCALL_OPENAT = 257,
    SYSCALL_MKDIRAT = 258,
    SYSCALL_UNLINKAT = 263,
    SYSCALL_LINKAT = 265,
    SYSCALL_READLINKAT = 267,
    SYSCALL_FACCESAT = 269
};

using syscall_t = void (*)(registers_t *);

extern bool initialised;

[[gnu::naked]] static inline syscall_ret syscall_i(size_t number, ...)
{
    asm volatile (
        "push %rbp \n\t"
        "mov %rsp, %rbp \n\t"
        "mov %rdi, %rax \n\t"
        "mov %rsi, %rdi \n\t"
        "mov %rdx, %rsi \n\t"
        "mov %rcx, %rdx \n\t"
        "mov %r8, %r10 \n\t"
        "mov %r9, %r8 \n\t"
        "mov 8(%rsp), %r9 \n\t"
        "int $0x69 \n\t"
        "mov %rbp, %rsp \n\t"
        "pop %rbp \n\t"
        "ret"
    );
}

[[gnu::naked]] static inline syscall_ret syscall_s(size_t number, ...)
{
    asm volatile (
        "push %rbp \n\t"
        "mov %rsp, %rbp \n\t"
        "mov %rdi, %rax \n\t"
        "mov %rsi, %rdi \n\t"
        "mov %rdx, %rsi \n\t"
        "mov %rcx, %rdx \n\t"
        "mov %r8, %r10 \n\t"
        "mov %r9, %r8 \n\t"
        "mov 8(%rsp), %r9 \n\t"
        "syscall \n\t"
        "mov %rbp, %rsp \n\t"
        "pop %rbp \n\t"
        "ret"
    );
}

extern "C" syscall_t syscall_table[];
extern "C" void syscall_entry();

void reboot(std::string message);
void init();
}