#include <stddef.h>
#include "../kernel.hpp"
#include "../stivale2.h"
#include "../include/string.hpp"

void (*term_write)(const char *string, int length);

void term_print(const char *string)
{
    term_write(string, strlen(string));
}

void term_init(struct stivale2_struct_tag_terminal *term_str_tag)
{
    void *term_write_ptr = (void *)term_str_tag->term_write;
    term_write = (void (*)(const char *string, int length))term_write_ptr;
}