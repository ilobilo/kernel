// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stdint.h>

#define CPUID_INVARIANT_TSC (1 << 8)
#define CPUID_TSC_DEADLINE (1 << 24)
#define CPUID_SMEP (1 << 7)
#define CPUID_SMAP (1 << 20)
#define CPUID_UMIP (1 << 2)
#define CPUID_X2APIC (1 << 21)
#define CPUID_GBPAGE (1 << 26)

struct [[gnu::packed]] registers_t
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, error_code, rip, cs, rflags, rsp, ss;
};

uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t value);

void set_kernel_gs(uintptr_t addr);
void set_user_gs(uintptr_t addr);
void set_user_fs(uintptr_t addr);

uintptr_t get_user_gs();
uintptr_t get_user_fs();

void write_cr(uint64_t reg, uint64_t val);
uint64_t read_cr(uint64_t reg);

void wrxcr(uint32_t i, uint64_t value);

void xsave(void *region);
void xrstor(void *region);
void fxsave(void *region);
void fxrstor(void *region);

void enableSSE();
void enableSMEP();
void enableSMAP();
void enableUMIP();