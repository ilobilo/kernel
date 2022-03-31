// Copyright (C) 2021-2022  ilobilo

#pragma region include
#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <drivers/block/drivemgr/drivemgr.hpp>
#include <drivers/net/rtl8139/rtl8139.hpp>
#include <drivers/net/rtl8169/rtl8169.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <drivers/net/nicmgr/nicmgr.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <drivers/audio/pcspk/pcspk.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <drivers/net/e1000/e1000.hpp>
#include <drivers/block/ahci/ahci.hpp>
#include <drivers/fs/tmpfs/tmpfs.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <drivers/vmware/vmware.hpp>
#include <drivers/block/ata/ata.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <system/acpi/acpi.hpp>
#include <drivers/ps2/ps2.hpp>
#include <system/pci/pci.hpp>
#include <system/vfs/vfs.hpp>
#include <kernel/kernel.hpp>
#include <lai/helpers/sci.h>
#include <apps/kshell.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/panic.hpp>
#include <lib/alloc.hpp>
#include <lib/ring.hpp>
#include <lib/log.hpp>
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
    auto ssfn_mod = find_module("sfn");
    if (ssfn_mod == nullptr) return;
    ssfn::init(reinterpret_cast<uint64_t>(ssfn_mod->address));

    while (true)
    {
        uint64_t free = pmm::freemem() / 1024;
        uint64_t used = pmm::usedmem() / 1024;

        ssfn::setcolour(ssfn::fgcolour, 0x227AD3);
        ssfn::resetpos();

        ssfn::printf("Current RTC time: %s\n", rtc::getTime());
        ssfn::printf("Total usable RAM: %ld KB, Used RAM: %ld KB\n", free + used, used);
        ssfn::printf("Process count: %zu, Thread count: %zu", scheduler::proc_count, scheduler::thread_count);
    }
}

using constructor_t = void (*)();

extern "C" constructor_t __init_array_start[];
extern "C" constructor_t __init_array_end[];
void constructors_init()
{
    for (constructor_t *ctor = __init_array_start; ctor < __init_array_end; ctor++) (*ctor)();
}

void main()
{
    log("Welcome to kernel project");
    terminal::center("Welcome to kernel project");

    if (!strcmp(KERNEL_VERSION, "0")) log("Git version: %s\n", GIT_VERSION);
    else log("Version: %s\n", KERNEL_VERSION);
    if (!strcmp(KERNEL_VERSION, "0")) printf("Git version: %s\n", GIT_VERSION);
    else printf("Version: %s\n", KERNEL_VERSION);

    log("CPU cores available: %ld", smp_request.response->cpu_count);
    log("Total usable memory: %ld MB\n", getmemsize() / 1024 / 1024);
    printf("CPU cores available: %ld\n", smp_request.response->cpu_count);
    printf("Total usable memory: %ld MB\n", getmemsize() / 1024 / 1024);

    log("Kernel cmdline: %s", cmdline);
    log("Available kernel modules:");
    for (size_t i = 0; i < module_request.response->module_count; i++)
    {
        log("%zu) %s", i, module_request.response->modules[i]->cmdline);
    }
    serial::newline();

    terminal::check("Initialising PMM...", reinterpret_cast<uint64_t>(pmm::init), -1, pmm::initialised);
    terminal::check("Initialising VMM...", reinterpret_cast<uint64_t>(vmm::init), -1, vmm::initialised);
    constructors_init();

    terminal::check("Initialising GDT...", reinterpret_cast<uint64_t>(gdt::init), -1, gdt::initialised);
    terminal::check("Initialising IDT...", reinterpret_cast<uint64_t>(idt::init), -1, idt::initialised);

    terminal::check("Initialising ACPI...", reinterpret_cast<uint64_t>(acpi::init), -1, acpi::initialised);
    terminal::check("Initialising HPET...", reinterpret_cast<uint64_t>(hpet::init), -1, hpet::initialised);
    terminal::check("Initialising PIT...", reinterpret_cast<uint64_t>(pit::init), -1, pit::initialised);
    terminal::check("Initialising PCI...", reinterpret_cast<uint64_t>(pci::init), -1, pci::initialised);
    terminal::check("Initialising APIC...", reinterpret_cast<uint64_t>(apic::init), -1, apic::initialised);
    terminal::check("Initialising SMP...", reinterpret_cast<uint64_t>(smp::init), -1, smp::initialised);
    // lai_enable_acpi(apic::initialised ? 1 : 0);

    terminal::check("Initialising AHCI...", reinterpret_cast<uint64_t>(ahci::init), -1, ahci::initialised);
    terminal::check("Initialising ATA...", reinterpret_cast<uint64_t>(ata::init), -1, ata::initialised);

    terminal::check("Initialising RTL8139...", reinterpret_cast<uint64_t>(rtl8139::init), -1, rtl8139::initialised);
    terminal::check("Initialising RTL8169...", reinterpret_cast<uint64_t>(rtl8169::init), -1, rtl8169::initialised);
    terminal::check("Initialising E1000...", reinterpret_cast<uint64_t>(e1000::init), -1, e1000::initialised);

    terminal::check("Initialising Drive Manager...", reinterpret_cast<uint64_t>(drivemgr::init), -1, drivemgr::initialised);
    terminal::check("Initialising NIC Manager...", reinterpret_cast<uint64_t>(nicmgr::init), -1, nicmgr::initialised);

    terminal::check("Initialising VFS...", reinterpret_cast<uint64_t>(vfs::init), -1, vfs::initialised);
    terminal::check("Initialising TMPFS...", reinterpret_cast<uint64_t>(tmpfs::init), -1, tmpfs::initialised);
    terminal::check("Initialising DEVFS...", reinterpret_cast<uint64_t>(devfs::init), -1, devfs::initialised);
    serial::init();

    auto initrd_mod = find_module("initrd");
    terminal::check("Initialising Initrd...", reinterpret_cast<uint64_t>(ustar::init), (initrd_mod ? reinterpret_cast<uint64_t>(initrd_mod->address) : 0), ustar::initialised, (initrd_mod && strstr(cmdline, "initrd")));

    terminal::check("Initialising System Calls...", reinterpret_cast<uint64_t>(syscall::init), -1, syscall::initialised);

    terminal::check("Initialising PS/2 Controller...", reinterpret_cast<uint64_t>(ps2::init), -1, ps2::initialised);

    terminal::check("Initialising VMWare Tools...", reinterpret_cast<uint64_t>(vmware::init), -1, vmware::initialised);

    printf("Current RTC time: %s\n\n", rtc::getTime());
    printf("Userspace has not been implemented yet! dropping to kernel shell...\n\n");

    auto proc = new scheduler::process_t("Init", reinterpret_cast<uint64_t>(apps::kshell::run), 0, scheduler::HIGH);
    proc->add_thread(reinterpret_cast<uint64_t>(time), 0, scheduler::LOW);
    proc->enqueue();

    scheduler::init();
}
}