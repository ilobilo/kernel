// Copyright (C) 2021  ilobilo

#pragma region include
#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/devices/ps2/mouse/mouse.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <drivers/audio/pcspk/pcspk.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <drivers/vmware/vmware.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/mm/heap/heap.hpp>
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
#include <stivale2.h>
#pragma endregion include

using namespace kernel::drivers::display;
using namespace kernel::drivers::audio;
using namespace kernel::drivers::fs;
using namespace kernel::drivers;
using namespace kernel::system::sched;
using namespace kernel::system::cpu;
using namespace kernel::system::mm;
using namespace kernel::system;

namespace kernel {

struct stivale2_struct_tag_smp *smp_tag;
struct stivale2_struct_tag_memmap *mmap_tag;
struct stivale2_struct_tag_rsdp *rsdp_tag;
struct stivale2_struct_tag_framebuffer *frm_tag;
struct stivale2_struct_tag_terminal *term_tag;
struct stivale2_struct_tag_modules *mod_tag;
struct stivale2_struct_tag_cmdline *cmd_tag;
struct stivale2_struct_tag_kernel_file_v2 *kfilev2_tag;
struct stivale2_struct_tag_hhdm *hhdm_tag;
struct stivale2_struct_tag_pmrs *pmrs_tag;
struct stivale2_struct_tag_kernel_base_address *kbaddr_tag;

char *cmdline;

int find_module(const char *name)
{
    for (uint64_t i = 0; i < mod_tag->module_count; i++)
    {
        if (!strcmp(mod_tag->modules[i].string, name)) return i;
    }
    return -1;
}

void main(struct stivale2_struct *stivale2_struct)
{
    smp_tag = (stivale2_struct_tag_smp (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);
    mmap_tag = (stivale2_struct_tag_memmap (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
    rsdp_tag = (stivale2_struct_tag_rsdp (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_RSDP_ID);
    frm_tag = (stivale2_struct_tag_framebuffer (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    term_tag = (stivale2_struct_tag_terminal (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID);
    mod_tag = (stivale2_struct_tag_modules (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MODULES_ID);
    cmd_tag = (stivale2_struct_tag_cmdline (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_CMDLINE_ID);
    kfilev2_tag = (stivale2_struct_tag_kernel_file_v2 (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_FILE_V2_ID);
    hhdm_tag = (stivale2_struct_tag_hhdm (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_HHDM_ID);
    pmrs_tag = (stivale2_struct_tag_pmrs (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_PMRS_ID);
    kbaddr_tag = (stivale2_struct_tag_kernel_base_address (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID);

    cmdline = (char *)cmd_tag->cmdline;

    if (!strstr(cmdline, "nocom")) serial::init();

    serial::info("Welcome to kernel project");

    if (!strcmp(KERNEL_VERSION, "0")) serial::info("Git version: %s\n", GIT_VERSION);
    else serial::info("Version: %s\n", KERNEL_VERSION);

    serial::info("CPU cores available: %d", smp_tag->cpu_count);
    serial::info("Total usable memory: %ld MB\n", getmemsize() / 1024 / 1024);
    serial::info("Arguments passed to kernel: %s", cmdline);

    serial::info("Available kernel modules:");
    for (uint64_t t = 0; t < mod_tag->module_count; t++)
    {
        serial::info("%d) %s", t + 1, mod_tag->modules[t].string);
    }
    serial::newline();

    if (frm_tag == NULL) PANIC("Could not find framebuffer tag!");
    framebuffer::init();
    ssfn::init();

    if (term_tag == NULL) PANIC("Could not find terminal tag!");
    terminal::init();

    terminal::center("Welcome to kernel project");

    if (!strcmp(KERNEL_VERSION, "0")) printf("Git version: %s\n", GIT_VERSION);
    else printf("Version: %s\n", KERNEL_VERSION);

    printf("CPU cores available: %ld\n", smp_tag->cpu_count);
    printf("Total usable memory: %ld MB\n", getmemsize() / 1024 / 1024);

    terminal::check("Initialising PMM...");
    pmm::init();
    terminal::okerr(pmm::initialised);

    terminal::check("Initialising VMM...");
    vmm::init();
    terminal::okerr(vmm::initialised);

    terminal::check("Initialising Heap...");
    heap::init();
    terminal::okerr(heap::initialised);

    terminal::check("Initialising Global Descriptor Table...");
    gdt::init();
    terminal::okerr(gdt::initialised);

    terminal::check("Initialising Interrupt Descriptor Table...");
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

    terminal::check("Initialising Virtual filesystem...");
    vfs::init();
    terminal::okerr(vfs::initialised);

    terminal::check("Initialising USTAR filesystem...");
    int m = find_module("initrd");
    if (m != -1 && strstr(cmdline, "initrd"))
    {
        ustar::init(mod_tag->modules[m].begin);
    }
    terminal::okerr(ustar::initialised);

    terminal::check("Initialising DEV filesystem...");
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

    srand(rtc::time());

    serial::info("Starting kernel shell\n");
    while (true) apps::kshell::run();
}
}