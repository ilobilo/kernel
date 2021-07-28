#include "include/string.hpp"
#include "drivers/terminal.hpp"
#include "include/io.hpp"
#include "system/gdt.hpp"
#include "stivale2.h"
#include "kernel.hpp"

void main(struct stivale2_struct *stivale2_struct)//struct stivale2_struct_tag_smp *smp)
{
    term_init((stivale2_struct_tag_terminal (*))stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID));
    term_center("Welcome to kernel project!");
    term_check(true, "Starting terminal...");
    term_check(GDT_init(), "Initializing Global Descriptot Table...");
}
