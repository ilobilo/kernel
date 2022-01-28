// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::system::trace {

struct stackframe_t
{
    stackframe_t *frame;
    uint64_t rip;
};

struct symtable_t
{
    uint64_t addr;
    const char *name;
};

void trace(bool terminal);
void init();
}