// Copyright (C) 2021  ilobilo

#pragma region include
#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <drivers/block/drivemgr/drivemgr.hpp>
#include <drivers/ps2/keyboard/keyboard.hpp>
#include <drivers/net/rtl8139/rtl8139.hpp>
#include <drivers/net/rtl8169/rtl8169.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <drivers/net/nicmgr/nicmgr.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <drivers/audio/pcspk/pcspk.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <drivers/net/e1000/e1000.hpp>
#include <drivers/block/ahci/ahci.hpp>
#include <drivers/ps2/mouse/mouse.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <drivers/vmware/vmware.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>
#include <kernel/kernel.hpp>
#include <apps/kshell.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/panic.hpp>
#include <lib/buddy.hpp>
#include <lib/log.hpp>
#include <stivale2.h>
#pragma endregion include

using namespace kernel::drivers::display;
using namespace kernel::drivers::audio;
using namespace kernel::drivers::block;
using namespace kernel::drivers::net;
using namespace kernel::drivers::fs;
using namespace kernel::drivers;
using namespace kernel::system::sched;
using namespace kernel::system::cpu;
using namespace kernel::system::mm;
using namespace kernel::system;

namespace kernel {

void time()
{
    while (true)
    {
        size_t size = 0;
        for (size_t i = 0; i < STACK_SIZE; i++)
        {
            if (kernel_stack[i] != 'A') break;
            size++;
        }

        uint64_t free = pmm::freemem() / 1024;

        ssfn::setcolour(ssfn::fgcolour, 0x227AD3);
        ssfn::printfat(0, 0, "\rCurrent RTC time: %s", rtc::getTime());
        ssfn::printfat(0, 1, "\rMaximum stack usage: %zu Bytes", STACK_SIZE - size);
        ssfn::printfat(0, 2, "\rFree RAM: %ld KB", free);
    }
}

extern "C" void (*__init_array_start)(), (*__init_array_end)();
void constructors_init()
{
    for (void (**ctor)() = &__init_array_start; ctor < &__init_array_end; ctor++) (*ctor)();
}

void main()
{
    log("Welcome to kernel project");
    terminal::center("Welcome to kernel project");

    if (!strcmp(KERNEL_VERSION, "0")) log("Git version: %s\n", GIT_VERSION);
    else log("Version: %s\n", KERNEL_VERSION);
    if (!strcmp(KERNEL_VERSION, "0")) printf("Git version: %s\n", GIT_VERSION);
    else printf("Version: %s\n", KERNEL_VERSION);

    log("CPU cores available: %d", smp_tag->cpu_count);
    log("Total usable memory: %ld MB\n", getmemsize() / 1024 / 1024);
    printf("CPU cores available: %ld\n", smp_tag->cpu_count);
    printf("Total usable memory: %ld MB\n", getmemsize() / 1024 / 1024);

    log("Kernel cmdline: %s", cmdline);
    log("Available kernel modules:");
    for (size_t i = 0; i < mod_tag->module_count; i++)
    {
        log("%zu) %s", i, mod_tag->modules[i].string);
    }
    serial::newline();

    terminal::check("Initialising PMM...");
    pmm::init();
    terminal::okerr(pmm::initialised);
    constructors_init();

    terminal::check("Initialising VMM...");
    vmm::init();
    terminal::okerr(vmm::initialised);

    terminal::check("Initialising GDT...");
    gdt::init();
    terminal::okerr(gdt::initialised);

    terminal::check("Initialising IDT...");
    idt::init();
    terminal::okerr(idt::initialised);

    terminal::check("Initialising ACPI...");
    acpi::init();
    terminal::okerr(acpi::initialised);

    terminal::check("Initialising HPET...");
    hpet::init();
    terminal::okerr(hpet::initialised);

    terminal::check("Initialising PCI...");
    pci::init();
    terminal::okerr(pci::initialised);

    terminal::check("Initialising APIC...");
    apic::init();
    terminal::okerr(apic::initialised);

    terminal::check("Initialising SMP...");
    smp::init();
    terminal::okerr(smp::initialised);

    terminal::check("Initialising AHCI...");
    ahci::init();
    terminal::okerr(ahci::initialised);

    terminal::check("Initialising RTL8139...");
    rtl8139::init();
    terminal::okerr(rtl8139::initialised);

    terminal::check("Initialising RTL8169...");
    rtl8169::init();
    terminal::okerr(rtl8169::initialised);

    terminal::check("Initialising E1000...");
    e1000::init();
    terminal::okerr(e1000::initialised);

    terminal::check("Initialising Drive Manager...");
    drivemgr::init();
    terminal::okerr(drivemgr::initialised);

    terminal::check("Initialising NIC Manager...");
    nicmgr::init();
    terminal::okerr(nicmgr::initialised);

    terminal::check("Initialising VFS...");
    vfs::init();
    terminal::okerr(vfs::initialised);

    terminal::check("Initialising Initrd...");
    int m = find_module("initrd");
    if (m != -1 && strstr(cmdline, "initrd"))
    {
        ustar::init(mod_tag->modules[m].begin);
    }
    terminal::okerr(ustar::initialised);

    terminal::check("Initialising DEVFS...");
    devfs::init();
    terminal::okerr(devfs::initialised);

    terminal::check("Initialising System calls...");
    syscall::init();
    terminal::okerr(syscall::initialised);

    terminal::check("Initialising PIT...");
    pit::init();
    terminal::okerr(pit::initialised);

    terminal::check("Initialising PS/2 Keyboard...");
    ps2::kbd::init();
    terminal::okerr(ps2::kbd::initialised);

    terminal::check("Initialising PS/2 Mouse...");
    if (!strstr(cmdline, "nomouse")) ps2::mouse::init();
    terminal::okerr(ps2::mouse::initialised);

    terminal::check("Initialising VMWare tools...");
    vmware::init();
    terminal::okerr(vmware::initialised);

    printf("Current RTC time: %s\n\n", rtc::getTime());
    printf("Userspace has not been implemented yet! dropping to kernel shell...\n\n");

    scheduler::proc_create("Init", reinterpret_cast<uint64_t>(apps::kshell::run), 0);
    scheduler::thread_create(reinterpret_cast<uint64_t>(time), 0, scheduler::initproc);

    scheduler::init();
}
}