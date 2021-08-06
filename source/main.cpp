#include "drivers/keyboard.hpp"
#include "drivers/pit.hpp"
#include "drivers/terminal.hpp"
#include "system/gdt/gdt.hpp"
#include "system/idt/idt.hpp"
#include "include/io.hpp"
#include "stivale2.h"
#include "kernel.hpp"

void main(struct stivale2_struct *stivale2_struct)
{
    struct stivale2_struct_tag_smp *smp_tag = (stivale2_struct_tag_smp (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);

    term_init((stivale2_struct_tag_terminal (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID));

    term_center("Welcome to kernel project!");

    term_print("CPU cores available: ");
    term_printi(smp_tag->cpu_count);
    term_print("\n");

    GDT_init();
    term_check(true, "Initializing Global Descriptor Table...");

    IDT_init();
    term_check(true, "Initializing Interrupt Descriptor Table...");

    PIT_init(100);
    term_check(true, "Initializing PIT...");

    Keyboard_init();
    term_check(true, "Initializing Keyboard...");

//    asm volatile ("int $0x3");
//    asm volatile ("int $0x4");
}
