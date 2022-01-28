// Copyright (C) 2021-2022  ilobilo

#include <lib/string.hpp>
#include <lib/mmio.hpp>
#include <lib/cpu.hpp>
#include <stdint.h>
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

void set_kernel_gs(uintptr_t addr)
{
    wrmsr(0xc0000101, addr);
}

void set_user_gs(uintptr_t addr)
{
    wrmsr(0xc0000102, addr);
}

void set_user_fs(uintptr_t addr)
{
    wrmsr(0xc0000100, addr);
}

uintptr_t get_user_gs()
{
    return rdmsr(0xc0000102);
}

uintptr_t get_user_fs()
{
    return rdmsr(0xc0000100);
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
    asm volatile("xsetbv" : : "a"(eax), "d"(edx), "c"(i) : "memory");
}

void xsave(void *region)
{
    asm volatile("xsave %0" : "+m"(FLAT_PTR(region)) : "a"(0xFFFFFFFF), "d"(0xFFFFFFFF) : "memory");
}

void xrstor(void *region)
{
    asm volatile("xrstor %0" :: "m"(FLAT_PTR(region)), "a"(0xFFFFFFFF), "d"(0xFFFFFFFF) : "memory");
}

void fxsave(void *region)
{
    asm volatile("fxsave %0" : "+m"(FLAT_PTR(region)) : : "memory");
}

void fxrstor(void *region)
{
    asm volatile("fxrstor %0" : : "m"(FLAT_PTR(region)) : "memory");
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
        if ((b & CPUID_SMEP))
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
        if ((b & CPUID_SMAP))
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
        if ((c & CPUID_UMIP))
        {
            write_cr(4, read_cr(4) | (1 << 11));
        }
    }
}