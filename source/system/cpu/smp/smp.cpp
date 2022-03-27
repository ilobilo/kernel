// Copyright (C) 2021-2022  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <system/cpu/syscall/syscall.hpp>
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
cpu_t *cpus = new cpu_t[smp_request.response->cpu_count]();

static void cpu_init(limine_smp_info *cpu)
{
    if (cpu->lapic_id != smp_request.response->bsp_lapic_id)
    {
        asm volatile ("mov %%rsp, %0" : "=r"(gdt::tss[reinterpret_cast<cpu_t*>(cpu->extra_argument)->id].RSP[0]) : : "memory");
    }

    cpu_lock.lock();
    set_kernel_gs(cpu->extra_argument);
    set_gs(cpu->extra_argument);

    gdt::reloadall(this_cpu->id);
    idt::reload();

    vmm::kernel_pagemap->switchTo();

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

    wrmsr(0xC0000080, rdmsr(0xC0000080) | (1 << 0));
    wrmsr(0xC0000081, 0x33002800000000);
    wrmsr(0xC0000082, reinterpret_cast<uint64_t>(syscall::syscall_entry));
    wrmsr(0xC0000084, ~static_cast<uint32_t>(0x02));

    log("CPU %ld is up", this_cpu->id);
    this_cpu->is_up = true;

    cpu_lock.unlock();
    if (cpu->lapic_id != smp_request.response->bsp_lapic_id)
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

    for (size_t i = 0; i < smp_request.response->cpu_count; i++)
    {
        limine_smp_info *smp_info = smp_request.response->cpus[i];
        smp_info->extra_argument = reinterpret_cast<uint64_t>(&cpus[i]);
        cpus[i].id = i;

        uint64_t sched_stack = malloc<uint64_t>(STACK_SIZE);
        gdt::tss[i].IST[0] = sched_stack + STACK_SIZE + hhdm_offset;

        if (smp_request.response->bsp_lapic_id != smp_info->lapic_id)
        {
            smp_request.response->cpus[i]->goto_address = cpu_init;
            while (cpus[i].is_up == false);
        }
        else cpu_init(smp_info);
    }

    log("All CPUs are up\n");
    initialised = true;
}
}