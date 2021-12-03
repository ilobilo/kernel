// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

struct cpu_context
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax, rip;
};

uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t value);