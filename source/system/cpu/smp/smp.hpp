// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/sched/scheduler/scheduler.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <lib/errno.hpp>
#include <cstddef>

using namespace kernel::system::sched;

namespace kernel::system::cpu::smp {

struct cpu_t
{
    uint64_t id;
    uint32_t lapic_id;
    gdt::TSS *tss;

    size_t fpu_storage_size;
    void (*fpu_save)(uint8_t*);
    void (*fpu_restore)(uint8_t*);

    scheduler::thread_t *current_thread;
    scheduler::process_t *current_proc;
    scheduler::process_t *idle_proc;

    errno_t err;

    volatile bool is_up;
};

extern bool initialised;
extern cpu_t *cpus;

#define this_cpu ({ &kernel::system::cpu::smp::cpus[read_gs(0)]; })

void init();
}