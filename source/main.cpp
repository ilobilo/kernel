#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/drawing/drawing.hpp>
#include <drivers/devices/ps2/mouse/mouse.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/pmindexer/pmindexer.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/mm/paging/paging.hpp>
#include <system/mm/bitmap/bitmap.hpp>
#include <system/power/acpi/acpi.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/timers/pit/pit.hpp>
#include <system/timers/rtc/rtc.hpp>
#include <system/memory/memory.hpp>
#include <system/cpu/gdt/gdt.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/heap/heap.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <misc/kshell.hpp>
#include <lib/io.hpp>
#include <stivale2.h>
#include <kernel.hpp>

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
    for (int i = 0; i < mod_tag->module_count; i++)
    {
        if (!strcmp(mod_tag->modules[i].string, "initrd")) return i;
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

    if (!strstr(cmdline, "nocom")) serial_init();

    serial_info("Welcome to kernel project");

    if (KERNEL_VERSION == "0") serial_info("Git version: %s\n", GIT_VERSION);
    else serial_info("Version: %s\n", KERNEL_VERSION);

    serial_info("CPU cores available: %d", smp_tag->cpu_count);
    serial_info("Total usable memory: %s\n", humanify(getmemsize()));
    serial_info("Arguments passed to kernel: %s", cmdline);

    serial_info("Available kernel modules:");
    for (int t = 0; t < mod_tag->module_count; t++)
    {
        serial_info("%d) %s", t + 1, mod_tag->modules[t].string);
    }
    serial_newline();

    if (frm_tag == NULL)
    {
        serial_err("Framebuffer has not been initialised!");
        serial_err("System halted!\n");
        while (true) asm volatile ("cli; hlt");
    }
    drawing_init();

    if (term_tag == NULL)
    {
        serial_err("Terminal has not been initialised!");
        serial_err("System halted!\n");
        while (true) asm volatile ("cli; hlt");
    }
    term_init();

    term_center("Welcome to kernel project");

    if (KERNEL_VERSION == "0") printf("Git version: %s\n", GIT_VERSION);
    else printf("Version: %s\n", KERNEL_VERSION);

    printf("CPU cores available: %d\n", smp_tag->cpu_count);
    printf("Total usable memory: %s\n", humanify(getmemsize()));

    int i = find_module("initrd");
    if (i != -1 && strstr(cmdline, "initrd"))
    {
        initrd_init(mod_tag->modules[i].begin + 0xFFFF800000000000);
    }
    term_check(initrd_initialised, "Initialising USTAR Initrd...");

    GDT_init();
    term_check(gdt_initialised, "Initialising Global Descriptor Table...");

    PFAlloc_init();
    term_check(pfalloc_initialised, "Initialising Page Frame Allocator...");

    PTManager_init();
    term_check(ptmanager_initialised, "Initialising Page Table Manager...");

    Heap_init();
    term_check(heap_initialised, "Initialising Kernel Heap...");

    IDT_init();
    term_check(idt_initialised, "Initialising Interrupt Descriptor Table...");

    ACPI_init();
    term_check(acpi_initialised, "Initialising ACPI...");

    PCI_init();
    term_check(pci_initialised, "Initialising PCI...");

    PIT_init();
    term_check(pit_initialised, "Initialising PIT...");

    Keyboard_init();
    term_check(kbd_initialised, "Initialising PS2 Keyboard...");

    if (!strstr(cmdline, "nomouse"))
    {
        if (strstr(cmdline, "oldmouse"))
        {
            mousebordercol = 0x000000;
            mouseinsidecol = 0xFFFFFF;
        }
        Mouse_init();
        term_check(mouse_initialised, "Initialising PS2 Mouse...");
    }

    printf("Current RTC time: %s", RTC_GetTime());

    printf("\n\nUserspace has not been implemented yet! dropping to kernel shell...\n\n");

    getchar();
    serial_info("Starting kernel shell\n");
    while (true) shell_run();
}
