// Copyright (C) 2021-2022  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <kernel/kernel.hpp>
#include <lib/alloc.hpp>
#include <lib/panic.hpp>
#include <lib/lock.hpp>
#include <lib/cpu.hpp>
#include <lib/log.hpp>
#include <cpuid.h>

using namespace kernel::system::mm;

namespace kernel::system::cpu::smp {

bool initialised = false;

new_lock(cpu_lock);
cpu_t *cpus = new cpu_t[smp_tag->cpu_count]();
static size_t i = 0;

static void cpu_init(stivale2_smp_info *cpu)
{
    cpu_lock.lock();
    gdt::reloadall(i);
    idt::reload();

    vmm::kernel_pagemap->switchTo();
    set_kernel_gs(static_cast<uintptr_t>(cpu->extra_argument));
    set_user_gs(static_cast<uintptr_t>(cpu->extra_argument));

    this_cpu->lapic_id = cpu->lapic_id;
    this_cpu->tss = &gdt::tss[this_cpu->id];

    enableSSE();
    enableSMEP();
    enableSMAP();
    enableUMIP();
    enablePAT();

    uint32_t a = 0, b = 0, c = 0, d = 0;
    __get_cpuid(1, &a, &b, &c, &d);
    if (c & bit_XSAVE)
    {
        write_cr(4, read_cr(4) | (1 << 18));

        // uint64_t xcr0 = (0 | (1 << 0)) | (1 << 1);
        // if (c & bit_AVX) xcr0 |= (1 << 2);

        // if (__get_cpuid(7, &a, &b, &c, &d))
        // {
        //     if (b & bit_AVX512F)
        //     {
        //         xcr0 |= (1 << 5);
        //         xcr0 |= (1 << 6);
        //         xcr0 |= (1 << 7);
        //     }
        // }
        // wrxcr(0, xcr0);

        assert(__get_cpuid_count(0x0D, 0, &a, &b, &c, &d), "CPUID failure");
        this_cpu->fpu_storage_size = c;
        this_cpu->fpu_restore = xrstor;

        assert(__get_cpuid_count(0x0D, 1, &a, &b, &c, &d), "CPUID failure");
        if (a & bit_XSAVEOPT) this_cpu->fpu_save = xsaveopt;
        else this_cpu->fpu_save = xsave;
    }
    else if (d & bit_FXSAVE)
    {
        write_cr(4, read_cr(4) | (1 << 9));

        this_cpu->fpu_storage_size = 512;
        this_cpu->fpu_save = fxsave;
        this_cpu->fpu_restore = fxrstor;
    }
    else panic("No known SIMD save mechanism");

    log("CPU %ld is up", this_cpu->id);
    this_cpu->is_up = true;

    cpu_lock.unlock();
    if (cpu->lapic_id != smp_tag->bsp_lapic_id)
    {
        if (apic::initialised) apic::lapic_init(this_cpu->lapic_id);
        scheduler::init();
        while (true) asm volatile ("hlt");
    }
}

void init()
{
    log("Initialising SMP");

    if (initialised)
    {
        warn("CPUs are already up!\n");
        return;
    }

    for (; i < smp_tag->cpu_count; i++)
    {
        smp_tag->smp_info[i].extra_argument = reinterpret_cast<uint64_t>(&cpus[i]);
        cpus[i].id = i;

        uint64_t sched_stack = malloc<uint64_t>(STACK_SIZE);
        gdt::tss[i].IST[0] = sched_stack + STACK_SIZE + hhdm_tag->addr;

        if (smp_tag->bsp_lapic_id != smp_tag->smp_info[i].lapic_id)
        {
            uint64_t stack = malloc<uint64_t>(STACK_SIZE);
            gdt::tss[i].RSP[0] = stack + STACK_SIZE + hhdm_tag->addr;

            smp_tag->smp_info[i].target_stack = stack + STACK_SIZE + hhdm_tag->addr;
            smp_tag->smp_info[i].goto_address = reinterpret_cast<uintptr_t>(cpu_init);
            while (cpus[i].is_up == false);
        }
        else cpu_init(&smp_tag->smp_info[i]);
    }

    log("All CPUs are up\n");
    initialised = true;
}
}