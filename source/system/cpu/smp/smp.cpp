#include <drivers/display/serial/serial.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <stivale2.h>
#include <main.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::cpu;
using namespace kernel::system::mm;

namespace kernel::system::cpu::smp {

bool initialised = false;
volatile bool cpu_up = false;

static void ap_startup(stivale2_smp_info *cpu)
{
    serial::info("SMP: Started CPU: %d", cpu->lapic_id);
    cpu_up = true;
    while (true) asm ("hlt");
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
        if (smp_tag->bsp_lapic_id != smp_tag->smp_info[i].lapic_id)
        {
            uint64_t stack = (uint64_t)pfalloc::requestPage();
            uint64_t sched_stack = (uint64_t)pfalloc::requestPage();

            gdt::tss[i].RSP[0] = stack;
            gdt::tss[i].IST[1] = sched_stack;

            smp_tag->smp_info[i].target_stack = stack;
            smp_tag->smp_info[i].goto_address = (uintptr_t)ap_startup;
            while(!cpu_up);
            cpu_up = false;
        }
    }
    serial::newline();
    initialised = true;
}
}