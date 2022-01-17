// Copyright (C) 2021  ilobilo

#pragma once

#include <system/sched/scheduler/scheduler.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <stddef.h>

using namespace kernel::system::sched;

namespace kernel::system::cpu::smp {

struct cpu_t
{
    uint64_t id;
    uint32_t lapic_id;
    gdt::TSS *tss;

    size_t fpu_storage_size;
    void (*fpu_save)(void*);
    void (*fpu_restore)(void*);

    scheduler::thread_t *current_thread;
    scheduler::process_t *current_proc;
    scheduler::thread_t *idle_thread;

    bool is_up;
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