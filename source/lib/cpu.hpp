// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>

static constexpr uint64_t CPUID_INVARIANT_TSC = (1 << 8);
static constexpr uint64_t CPUID_TSC_DEADLINE = (1 << 24);
static constexpr uint64_t CPUID_SMEP = (1 << 7);
static constexpr uint64_t CPUID_SMAP = (1 << 20);
static constexpr uint64_t CPUID_UMIP = (1 << 2);
static constexpr uint64_t CPUID_X2APIC = (1 << 21);
static constexpr uint64_t CPUID_GBPAGE = (1 << 26);

enum PAT
{
    Uncachable = 0x00,
    WriteCombining = 0x01,
    WriteThrough = 0x04,
    WriteProtected = 0x05,
    WriteBack = 0x06,
    Uncached = 0x07
};

struct [[gnu::packed]] registers_t
{
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, error_code, rip, cs, rflags, rsp, ss;
};

uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t value);

void set_kernel_gs(uint64_t addr);
uint64_t get_kernel_gs();

void set_gs(uint64_t addr);
uint64_t get_gs();

void set_fs(uint64_t addr);
uint64_t get_fs();

void write_cr(uint64_t reg, uint64_t val);
uint64_t read_cr(uint64_t reg);

void wrxcr(uint32_t i, uint64_t value);

void xsaveopt(uint8_t *region);
void xsave(uint8_t *region);
void xrstor(uint8_t *region);
void fxsave(uint8_t *region);
void fxrstor(uint8_t *region);

void invlpg(uint64_t addr);

void enableSSE();
void enableSMEP();
void enableSMAP();
void enableUMIP();
void enablePAT();

#define read_gs(offset) \
({ \
    uint64_t value; \
    asm volatile("movq %%gs:[" #offset "], %0" : "=r"(value) : : "memory"); \
    value; \
})