#include <drivers/keyboard/keyboard.hpp>
#include <drivers/pit/pit.hpp>
#include <drivers/terminal/terminal.hpp>
#include <drivers/serial/serial.hpp>
#include <drivers/drawing/drawing.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/memory/memory.hpp>
#include <system/gdt/gdt.hpp>
#include <system/idt/idt.hpp>
#include <system/rtc/rtc.hpp>
#include <include/string.hpp>
#include <include/io.hpp>
#include <misc/kshell.hpp>
#include <stivale2.h>
#include <kernel.hpp>

struct stivale2_struct_tag_smp *smp_tag;
struct stivale2_struct_tag_memmap *mmap_tag;
struct stivale2_struct_tag_framebuffer *frm_tag;
struct stivale2_struct_tag_terminal *term_tag;
struct stivale2_struct_tag_modules *mod_tag;
struct stivale2_struct_tag_cmdline *cmd_tag;

void main(struct stivale2_struct *stivale2_struct)
{
    smp_tag = (stivale2_struct_tag_smp (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);
    mmap_tag = (stivale2_struct_tag_memmap (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
    frm_tag = (stivale2_struct_tag_framebuffer (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    term_tag = (stivale2_struct_tag_terminal (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID);
    mod_tag = (stivale2_struct_tag_modules (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MODULES_ID);
    cmd_tag = (stivale2_struct_tag_cmdline (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_CMDLINE_ID);

    serial_init();

    char* cmdline = (char *)cmd_tag->cmdline;
    bool initrd = false;

    if (strstr(cmdline, "initrd"))
    {
        initrd_init(mod_tag->modules->begin, mod_tag->modules->end);
        initrd = true;
    }

    drawing_init();
    term_init();

    term_center("Welcome to kernel project");

    printf("CPU cores available: %d\n", smp_tag->cpu_count);
    printf("Total memory: %dMB\n", getmemsize() / 1024 / 1024);
    printf("Total usable memory: %dMB\n", getusablememsize() / 1024 / 1024);

    GDT_init();
    term_check(true, "Initializing Global Descriptor Table...");

    IDT_init();
    term_check(true, "Initializing Interrupt Descriptor Table...");

    PIT_init();
    term_check(true, "Initializing PIT...");

    Keyboard_init();
    term_check(true, "Initializing Keyboard...");

    printf("Current RTC time: ");
    RTC_GetTime();

    printf("\nUserspace not yet implemented! dropping to kernel shell...\n\n");

    serial_info("Starting kernel shell\n");
    while (true)
    {
        shell_run();
    }
}
