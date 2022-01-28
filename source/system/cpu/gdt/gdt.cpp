// Copyright (C) 2021-2022  ilobilo

#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <kernel/kernel.hpp>
#include <lib/memory.hpp>
#include <lib/alloc.hpp>
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

void reloadall(size_t cpu)
{
    gdt_lock.lock();

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

    tss[cpu].IOPBOffset = sizeof(TSS);

    reloadgdt();
    reloadtss();

    gdt_lock.unlock();
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

    reloadall(0);
    tss[0].RSP[0] = reinterpret_cast<uint64_t>(kernel_stack + STACK_SIZE);

    serial::newline();
    initialised = true;
}
}