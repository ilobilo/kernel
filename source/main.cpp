#include "drivers/terminal.hpp"
#include "system/gdt/gdt.hpp"
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

    term_check(GDT_init(), "Initializing Global Descriptor Table...");
}
