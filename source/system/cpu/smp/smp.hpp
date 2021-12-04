// Copyright (C) 2021  ilobilo

#pragma once

#include <system/cpu/gdt/gdt.hpp>

namespace kernel::system::cpu::smp {

struct cpu_t
{
    uint64_t id;
    uint32_t lapic_id;
    gdt::TSS *tss;
    volatile bool up;
};

extern cpu_t *cpus;

#define this_cpu \
({ \
    uint64_t cpu_number; \
    asm volatile("movq %%gs:[0], %0" : "=r"(cpu_number) : : "memory"); \
    &kernel::system::cpu::smp::cpus[cpu_number]; \
})

extern bool initialised;

void init();
}