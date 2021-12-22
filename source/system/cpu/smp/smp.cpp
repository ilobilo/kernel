// Copyright (C) 2021  ilobilo

#include <system/cpu/apic/apic.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <kernel/kernel.hpp>
#include <lib/buddy.hpp>
#include <lib/lock.hpp>
#include <lib/cpu.hpp>
#include <lib/log.hpp>
#include <cpuid.h>

using namespace kernel::system::mm;

namespace kernel::system::cpu::smp {

bool initialised = false;

DEFINE_LOCK(cpu_lock)
volatile int cpus_up = 0;
cpu_t *cpus;

extern "C" void InitSSE();
static void cpu_init(stivale2_smp_info *cpu)
{
    acquire_lock(cpu_lock);
    gdt::reloadall(cpu->lapic_id);
    idt::reload();

    vmm::switchPagemap(vmm::kernel_pagemap);

    set_kernel_gs(static_cast<uintptr_t>(cpu->extra_argument));
    set_user_gs(static_cast<uintptr_t>(cpu->extra_argument));

    this_cpu->lapic_id = cpu->lapic_id;
    this_cpu->tss = &gdt::tss[this_cpu->lapic_id];

    enableSSE();
    enableSMEP();
    enableSMAP();
    enableUMIP();
    
    uint64_t cr4 = 0;
    uint32_t a = 0, b = 0, c = 0, d = 0;
    __get_cpuid(1, &a, &b, &c, &d);
    if ((c & bit_XSAVE))
    {
        cr4 = read_cr(4);
        cr4 |= (1 << 18);
        write_cr(4, cr4);
        
        uint64_t xcr0 = 0;
        xcr0 |= (1 << 0);
        xcr0 |= (1 << 1);
        
        if ((c & bit_AVX)) xcr0 |= (1 << 2);
        
        if (__get_cpuid(7, &a, &b, &c, &d))
        {
            if ((b & bit_AVX512F))
            {
                xcr0 |= (1 << 5);
                xcr0 |= (1 << 6);
                xcr0 |= (1 << 7);
            }
        }
        wrxcr(0, xcr0);
        
        this_cpu->fpu_storage_size = c;
        
        this_cpu->fpu_save = xsave;
        this_cpu->fpu_restore = xrstor;
	}
    else
    {
        this_cpu->fpu_storage_size = 512;
        this_cpu->fpu_save = fxsave;
        this_cpu->fpu_restore = fxrstor;
	}

    log("CPU %ld is up", this_cpu->lapic_id);
    this_cpu->up = true;
    cpus_up++;

    release_lock(cpu_lock);
    if (cpu->lapic_id != smp_tag->bsp_lapic_id)
    {
        if (apic::initialised) apic::lapic_init(this_cpu->lapic_id);
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

    cpus = static_cast<cpu_t*>(calloc(smp_tag->cpu_count, sizeof(cpu_t)));

    for (size_t i = 0; i < smp_tag->cpu_count; i++)
    {
        smp_tag->smp_info[i].extra_argument = (uint64_t)&cpus[i];

        uint64_t stack = reinterpret_cast<uint64_t>(pmm::alloc());
        uint64_t sched_stack = reinterpret_cast<uint64_t>(pmm::alloc());

        gdt::tss[i].RSP[0] = stack;
        gdt::tss[i].IST[1] = sched_stack;

        cpus[i].id = i;

        if (smp_tag->bsp_lapic_id != smp_tag->smp_info[i].lapic_id)
        {
            smp_tag->smp_info[i].target_stack = stack;
            smp_tag->smp_info[i].goto_address = reinterpret_cast<uintptr_t>(cpu_init);
        }
        else cpu_init(&smp_tag->smp_info[i]);
    }

    while (static_cast<uint64_t>(cpus_up) < smp_tag->cpu_count);

    log("All CPUs are up\n");
    initialised = true;
}
}