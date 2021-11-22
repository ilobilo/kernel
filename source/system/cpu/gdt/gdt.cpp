// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/sched/lock/lock.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <lib/memory.hpp>
#include <main.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

namespace kernel::system::cpu::gdt {

__attribute__((aligned(0x1000)))
GDT DefaultGDT = {
    {0, 0, 0, 0x00, 0x00, 0},
    {0xffff, 0, 0, 0x9a, 0x80, 0},
    {0xffff, 0, 0, 0x92, 0x80, 0},
    {0xffff, 0, 0, 0x9a, 0xcf, 0},
    {0xffff, 0, 0, 0x92, 0xcf, 0},
    {0, 0, 0, 0x9a, 0xa2, 0},
    {0, 0, 0, 0x92, 0xa0, 0},
    {0, 0, 0, 0xF2, 0, 0},
    {0, 0, 0, 0xFA, 0x20, 0}
};

DEFINE_LOCK(gdt_lock);
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
    acquire_lock(&gdt_lock);

    uintptr_t base = (uintptr_t)&tss[cpu];
    uintptr_t limit = base + sizeof(tss[cpu]);

    DefaultGDT.Tss.Base0 = (base & 0xFFFF);
    DefaultGDT.Tss.Base1 = (base >> 16) & 0xFF;
    DefaultGDT.Tss.Base2 = base >> 24;

    DefaultGDT.Tss.Limit0 = (limit & 0xFFFF);
    DefaultGDT.Tss.Limit1_Flags = (limit >> 16) & 0x0F;
    DefaultGDT.Tss.Limit1_Flags |= 0x00 & 0xF0;

    DefaultGDT.Tss.AccessByte = 0xE9;

    reloadgdt();
    reloadtss();

    release_lock(&gdt_lock);
}

void init()
{
    serial::info("Initialising GDT");

    if (initialised)
    {
        serial::info("GDT has already been initialised!\n");
        return;
    }

    tss = (TSS*)heap::calloc(smp_tag->cpu_count, sizeof(TSS));

    gdtDescriptor.Size = sizeof(GDT) - 1;
    gdtDescriptor.Offset = (uint64_t)&DefaultGDT;

    reloadall(smp_tag->bsp_lapic_id);

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
}