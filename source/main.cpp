#include <drivers/keyboard/keyboard.hpp>
#include <drivers/pit/pit.hpp>
#include <drivers/terminal/terminal.hpp>
#include <drivers/fs/tar/tar.hpp>
#include <system/gdt/gdt.hpp>
#include <system/idt/idt.hpp>
#include <system/rtc/rtc.hpp>
#include <include/string.hpp>
#include <include/io.hpp>
#include <stivale2.h>
#include <kernel.hpp>

void main(struct stivale2_struct *stivale2_struct)
{
    struct stivale2_struct_tag_smp *smp_tag = (stivale2_struct_tag_smp (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);
    struct stivale2_struct_tag_modules *mod_tag = (stivale2_struct_tag_modules (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MODULES_ID);
    struct stivale2_struct_tag_cmdline *cmd_tag = (stivale2_struct_tag_cmdline (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_CMDLINE_ID);

    term_init((stivale2_struct_tag_terminal (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID));

    char* cmdline = (char *)cmd_tag->cmdline;
    bool initrd = false;
    if (strstr(cmdline, "initrd"))
    {
        tar_init(mod_tag->modules->begin);
        initrd = true;
    }

    term_center("Welcome to kernel project");

    printf("CPU cores available: %i\n", smp_tag->cpu_count);

    GDT_init();
    term_check(true, "Initializing Global Descriptor Table...");

    IDT_init();
    term_check(true, "Initializing Interrupt Descriptor Table...");

    PIT_init(100);
    term_check(true, "Initializing PIT...");

    Keyboard_init();
    term_check(true, "Initializing Keyboard...");

    printf("Current RTC time: ");
    RTC_GetTime();

    if (initrd)
    {
        printf("\n\033[31mExecuting tar_list() here:\n\n");
        term_resetcolour();

        tar_list();

        printf("\n\033[31mExecuting tar_cat(\"./example2.txt\") here:\n\n");
        term_resetcolour();

        tar_cat("./example2.txt");

        printf("\n\033[31mExecuting tar_cat(\"./example.txt\") here:\n\n");
        term_resetcolour();

        tar_cat("./example.txt");
    }

//    asm volatile ("int $0x3");
//    asm volatile ("int $0x4");
}
