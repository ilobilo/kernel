// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/ustar/ustar.hpp>
#include <drivers/fs/ilar/ilar.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::fs::initrd {

bool initialised = false;

void init(uint64_t module)
{
    ilar::init(module);
    if (ilar::initialised == false)
    {
        ustar::init(module);
        if (ustar::initialised == false) return;
    }

    initialised = true;
}
}