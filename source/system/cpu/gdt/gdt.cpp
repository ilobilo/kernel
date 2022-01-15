// Copyright (C) 2021  ilobilo

#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <kernel/kernel.hpp>
#include <lib/memory.hpp>
#include <lib/buddy.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>

namespace kernel::system::cpu::gdt {

[[gnu::aligned(0x1000)]] GDT DefaultGDT
{
    {0x0000, 0, 0, 0x00, 0x00, 0}, // NULL
    {0xFFFF, 0, 0, 0x9A, 0x00, 0}, // 16 Bit code
    {0xFFFF, 0, 0, 0x92, 0x00, 0}, // 16 Bit data
    {0xFFFF, 0, 0, 0x9A, 0xCF, 0}, // 32 Bit code
    {0xFFFF, 0, 0, 0x92, 0xCF, 0}, // 32 Bit data
    {0x0000, 0, 0, 0x9A, 0x20, 0}, // 64 Bit code
    {0x0000, 0, 0, 0x92, 0x00, 0}, // 64 Bit data
    {0x0000, 0, 0, 0xF2, 0x00, 0}, // User data
    {0x0000, 0, 0, 0xFA, 0x20, 0}, // User code
    {0x0000, 0, 0, 0x89, 0x00, 0, 0, 0} // TSS
};

DEFINE_LOCK(gdt_lock)
bool initialised = false;
GDTDescriptor gdtDescriptor;
TSS *tss;

void reloadgdt()
{
    LoadGDT(&gdtDescriptor);
}

void reloadtss()
{
    LoadTSS();
}

void reloadall(int cpu)
{
    acquire_lock(gdt_lock);

    uintptr_t base = reinterpret_cast<uintptr_t>(&tss[cpu]);
    uintptr_t limit = base + sizeof(tss[cpu]);

    DefaultGDT.Tss.Length = limit;

    DefaultGDT.Tss.Base0 = base;
    DefaultGDT.Tss.Base1 = base >> 16;
    DefaultGDT.Tss.Flags1 = 0x89;
    DefaultGDT.Tss.Flags2 = 0x00;
    DefaultGDT.Tss.Base2 = base >> 24;
    DefaultGDT.Tss.Base3 = base >> 32;
    DefaultGDT.Tss.Reserved = 0x00;

    reloadgdt();
    reloadtss();

    release_lock(gdt_lock);
}

void init()
{
    log("Initialising GDT");

    if (initialised)
    {
        warn("GDT has already been initialised!\n");
        return;
    }

    tss = static_cast<TSS*>(calloc(smp_tag->cpu_count, sizeof(TSS)));

    gdtDescriptor.Size = sizeof(GDT) - 1;
    gdtDescriptor.Offset = reinterpret_cast<uint64_t>(&DefaultGDT);

    reloadall(smp_tag->bsp_lapic_id);
    tss[0].RSP[0] = reinterpret_cast<uint64_t>(stack);

    serial::newline();
    initialised = true;
}

void set_stack(uint64_t cpu, uintptr_t stack)
{
    tss[cpu].RSP[0] = stack;
}

uint64_t get_stack(uint64_t cpu)
{
    return tss[cpu].RSP[0];
}

void set_stack(uintptr_t stack)
{
    tss[this_cpu->lapic_id].RSP[0] = stack;
}

uint64_t get_stack()
{
    return tss[this_cpu->lapic_id].RSP[0];
}
}