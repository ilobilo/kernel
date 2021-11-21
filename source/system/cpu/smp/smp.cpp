// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/sched/lock/lock.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <stivale2.h>
#include <main.hpp>
#include <lib/math.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

namespace kernel::system::cpu::smp {

bool initialised = false;

DEFINE_LOCK(cpu_lock)
cpu_t *cpus;

uint64_t rdmsr(uint32_t msr)
{
    uint32_t edx, eax;
    asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");
    return ((uint64_t)edx << 32) | eax;
}

void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t edx = value >> 32;
    uint32_t eax = (uint32_t)value;
    asm volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(msr) : "memory");
}

extern "C" void InitSSE();
static void cpu_init(stivale2_smp_info *cpu)
{
    acquire_lock(&cpu_lock);
    gdt::reloadall(cpu->lapic_id);
    idt::reload();

    ptmanager::switchPTable(ptmanager::globalPTManager.PML4);

    wrmsr(0xC0000101, (uintptr_t)cpu->extra_argument);

    InitSSE();

    this_cpu->lapic_id = cpu->lapic_id;
    this_cpu->tss = &gdt::tss[this_cpu->lapic_id];

    serial::info("CPU %ld is up", this_cpu->lapic_id);
    this_cpu->up = true;

    release_lock(&cpu_lock);
    if (cpu->lapic_id != smp_tag->bsp_lapic_id) while (true) asm volatile ("hlt");
}

void init()
{
    serial::info("Initialising SMP");

    if (initialised)
    {
        serial::info("CPUs are already up!\n");
        return;
    }

    cpus = (cpu_t*)heap::calloc(smp_tag->cpu_count, sizeof(cpu_t));

    for (size_t i = 0; i < smp_tag->cpu_count; i++)
    {
        smp_tag->smp_info[i].extra_argument = (uint64_t)&cpus[i];

        uint64_t stack = (uint64_t)pfalloc::requestPage();
        uint64_t sched_stack = (uint64_t)pfalloc::requestPage();

        gdt::tss[i].RSP[0] = stack;
        gdt::tss[i].IST[1] = sched_stack;

        cpus[i].id = i;

        if (smp_tag->bsp_lapic_id != smp_tag->smp_info[i].lapic_id)
        {
            smp_tag->smp_info[i].target_stack = stack;
            smp_tag->smp_info[i].goto_address = (uintptr_t)cpu_init;
        }
        else cpu_init(&smp_tag->smp_info[i]);
    }

    for (size_t i = 0; i < smp_tag->cpu_count; i++) while (!cpus[i].up);

    serial::info("All CPUs are up\n");
    initialised = true;
}
}