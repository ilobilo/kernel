#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/sched/lock/lock.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/idt/idt.hpp>
#include <stivale2.h>
#include <main.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::cpu;
using namespace kernel::system::mm;

namespace kernel::system::cpu::smp {

bool initialised = false;
volatile bool cpu_up = false;

extern "C" void InitSSE();
static void cpu_init(stivale2_smp_info *cpu)
{
    gdt::reloadall(cpu->lapic_id);
    idt::reload();

    ptmanager::switchPTable(ptmanager::globalPTManager.PML4);

    InitSSE();

    serial::info("SMP: CPU %d is up", cpu->lapic_id);

    if (cpu->lapic_id != smp_tag->bsp_lapic_id)
    {
        cpu_up = true;
        while (true) asm volatile ("hlt");
    }
}

void init()
{
    serial::info("Initialising SMP");

    if (initialised)
    {
        serial::info("SMP already initialised!\n");
        return;
    }
    if (smp_tag->cpu_count == 1)
    {
        serial::info("Can't initialise SMP! only one core available\n");
        return;
    }

    for (size_t i = 0; i < smp_tag->cpu_count; i++)
    {
        uint64_t stack = (uint64_t)pfalloc::requestPage();
        uint64_t sched_stack = (uint64_t)pfalloc::requestPage();

        gdt::tss[i].RSP[0] = stack;
        gdt::tss[i].IST[1] = sched_stack;

        if (smp_tag->bsp_lapic_id != smp_tag->smp_info[i].lapic_id)
        {
            smp_tag->smp_info[i].target_stack = stack;
            smp_tag->smp_info[i].goto_address = (uintptr_t)cpu_init;
            while (!cpu_up);
            cpu_up = false;
        }
        else cpu_init(&smp_tag->smp_info[i]);
    }

    serial::info("SMP: All CPUs are up\n");
    initialised = true;
}
}