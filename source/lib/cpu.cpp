// Copyright (C) 2021-2022  ilobilo

#include <lib/string.hpp>
#include <lib/mmio.hpp>
#include <lib/cpu.hpp>
#include <cstdint>
#include <cpuid.h>

uint64_t rdmsr(uint32_t msr)
{
    uint32_t edx, eax;
    asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");
    return (static_cast<uint64_t>(edx) << 32) | eax;
}

void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t edx = value >> 32;
    uint32_t eax = static_cast<uint32_t>(value);
    asm volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(msr) : "memory");
}

void set_kernel_gs(uint64_t addr)
{
    wrmsr(0xC0000102, addr);
}

uint64_t get_kernel_gs()
{
    return rdmsr(0xC0000102);
}

void set_gs(uint64_t addr)
{
    wrmsr(0xC0000101, addr);
}

uint64_t get_gs()
{
    return rdmsr(0xC0000101);
}

void set_fs(uint64_t addr)
{
    wrmsr(0xC0000100, addr);
}

uint64_t get_fs()
{
    return rdmsr(0xC0000100);
}

void write_cr(uint64_t reg, uint64_t val)
{
    switch (reg)
    {
        case 0:
            asm volatile ("mov %0, %%cr0" :: "r" (val) : "memory");
            break;
        case 2:
            asm volatile ("mov %0, %%cr2" :: "r" (val) : "memory");
            break;
        case 3:
            asm volatile ("mov %0, %%cr3" :: "r" (val) : "memory");
            break;
        case 4:
            asm volatile ("mov %0, %%cr4" :: "r" (val) : "memory");
            break;
    }
}

uint64_t read_cr(uint64_t reg)
{
    uint64_t cr;
    switch (reg)
    {
        case 0:
            asm volatile ("mov %%cr0, %0" : "=r" (cr) :: "memory");
            break;
        case 2:
            asm volatile ("mov %%cr2, %0" : "=r" (cr) :: "memory");
            break;
        case 3:
            asm volatile ("mov %%cr3, %0" : "=r" (cr) :: "memory");
            break;
        case 4:
            asm volatile ("mov %%cr4, %0" : "=r" (cr) :: "memory");
            break;
    }
    return cr;
}

void wrxcr(uint32_t i, uint64_t value)
{
    uint32_t edx = value >> 32;
    uint32_t eax = static_cast<uint64_t>(value);
    asm volatile ("xsetbv" : : "a"(eax), "d"(edx), "c"(i) : "memory");
}

static uint64_t rfbm = ~0ull;
static uint32_t rfbm_low = rfbm & 0xFFFF'FFFF;
static uint32_t rfbm_high = (rfbm >> 32) & 0xFFFF'FFFF;

void xsaveopt(uint8_t *region)
{
    asm volatile ("xsaveopt64 (%0)" : : "r"(region), "a"(rfbm_low), "d"(rfbm_high) : "memory");
}

void xsave(uint8_t *region)
{
    asm volatile ("xsaveq (%0)" : : "r"(region), "a"(rfbm_low), "d"(rfbm_high) : "memory");
}

void xrstor(uint8_t *region)
{
    asm volatile ("xrstorq (%0)" : : "r"(region), "a"(rfbm_low), "d"(rfbm_high) : "memory");
}

void fxsave(uint8_t *region)
{
    asm volatile ("fxsaveq (%0)" : : "r"(region) : "memory");
}

void fxrstor(uint8_t *region)
{
    asm volatile ("fxrstorq (%0)" : : "r"(region) : "memory");
}

void invlpg(uint64_t addr)
{
    asm volatile ("invlpg (%0)" : : "r"(addr));
}

void enableSSE()
{
    write_cr(0, (read_cr(0) & ~(1 << 2)) | (1 << 1));
    write_cr(4, read_cr(4) | (3 << 9));
}

void enableSMEP()
{
    uint32_t a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid(7, &a, &b, &c, &d))
    {
        if (b & CPUID_SMEP)
        {
            write_cr(4, read_cr(4) | (1 << 20));
        }
    }
}

void enableSMAP()
{
    uint32_t a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid(7, &a, &b, &c, &d))
    {
        if (b & CPUID_SMAP)
        {
            write_cr(4, read_cr(4) | (1 << 21));
            asm volatile ("clac");
        }
    }
}

void enableUMIP()
{
    uint32_t a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid(7, &a, &b, &c, &d))
    {
        if (c & CPUID_UMIP)
        {
            write_cr(4, read_cr(4) | (1 << 11));
        }
    }
}

void enablePAT()
{
    wrmsr(0x277, WriteBack | (Uncachable << 8) | (WriteCombining << 16));
}