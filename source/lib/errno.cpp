// Copyright (C) 2021-2022  ilobilo

#include <system/cpu/smp/smp.hpp>

void errno_set(errno err)
{
    this_cpu->err = err;
}

errno errno_get()
{
    return this_cpu->err;
}