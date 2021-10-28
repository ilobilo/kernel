#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/devices/ps2/mouse/mouse.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/pmindexer/pmindexer.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/mm/paging/paging.hpp>
#include <system/mm/bitmap/bitmap.hpp>
#include <system/power/acpi/acpi.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <drivers/font/font.hpp>
#include <system/pci/pci.hpp>
#include <apps/kshell.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/vector.hpp>
#include <lib/io.hpp>
#include <stivale2.h>
#include <kernel.hpp>
#include <ssfn.h>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;
using namespace kernel::drivers;
using namespace kernel::system::power;
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

char *cmdline;

int find_module(char *name)
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

    cmdline = (char *)cmd_tag->cmdline;

    if (!strstr(cmdline, "nocom")) serial::init();

    serial::info("Welcome to kernel project");

    if (!strcmp(KERNEL_VERSION, "0")) serial::info("Git version: %s\n", GIT_VERSION);
    else serial::info("Version: %s\n", KERNEL_VERSION);

    serial::info("CPU cores available: %d", smp_tag->cpu_count);
    serial::info("Total usable memory: %s\n", humanify(getmemsize()));
    serial::info("Arguments passed to kernel: %s", cmdline);

    serial::info("Available kernel modules:");
    for (uint64_t t = 0; t < mod_tag->module_count; t++)
    {
        serial::info("%d) %s", t + 1, mod_tag->modules[t].string);
    }
    serial::newline();

    if (frm_tag == NULL)
    {
        serial::err("Framebuffer has not been initialised!");
        serial::err("System halted!\n");
        while (true) asm volatile ("cli; hlt");
    }
    framebuffer::init();
    font::init();

    if (term_tag == NULL)
    {
        serial::err("Terminal has not been initialised!");
        serial::err("System halted!\n");
        while (true) asm volatile ("cli; hlt");
    }
    terminal::init();

    terminal::center("Welcome to kernel project");

    if (!strcmp(KERNEL_VERSION, "0")) printf("Git version: %s\n", GIT_VERSION);
    else printf("Version: %s\n", KERNEL_VERSION);

    printf("CPU cores available: %ld\n", smp_tag->cpu_count);
    printf("Total usable memory: %s\n", humanify(getmemsize()));

    pfalloc::init();
    terminal::check(pfalloc::initialised, "Initialising Page Frame Allocator...");

    ptmanager::init();
    terminal::check(ptmanager::initialised, "Initialising Page Table Manager...");

    heap::init();
    terminal::check(heap::initialised, "Initialising Kernel Heap...");

    vfs::init();
    terminal::check(vfs::initialised, "Initialising Virtual filesystem...");

    int i = find_module("initrd");
    if (i != -1 && strstr(cmdline, "initrd"))
    {
        ustar::init(mod_tag->modules[i].begin);
    }
    terminal::check(ustar::initialised, "Initialising USTAR filesystem...");

    gdt::init();
    terminal::check(gdt::initialised, "Initialising Global Descriptor Table...");

    idt::init();
    terminal::check(idt::initialised, "Initialising Interrupt Descriptor Table...");

    smp::init();
    terminal::check(smp::initialised, "Initialising SMP...");

    acpi::init();
    terminal::check(acpi::initialised, "Initialising ACPI...");

    if (strstr(cmdline, "pciids")) pci::use_pciids = true;
    pci::init();
    terminal::check(pci::initialised, "Initialising PCI...");

    pit::init();
    terminal::check(pit::initialised, "Initialising PIT...");

    ps2::kbd::init();
    terminal::check(ps2::kbd::initialised, "Initialising PS2 Keyboard...");

    if (!strstr(cmdline, "nomouse")) ps2::mouse::init();
    terminal::check(ps2::mouse::initialised, "Initialising PS2 Mouse...");

    printf("Current RTC time: %s", rtc::getTime());

    printf("\n\nUserspace has not been implemented yet! dropping to kernel shell...\n\n");

    ps2::kbd::getchar();
    serial::info("Starting kernel shell\n");
    while (true) apps::kshell::run();
}
}