// Copyright (C) 2021  ilobilo

#include <kernel/kernel.hpp>
#include <stdint.h>
#include <stddef.h>

uint64_t getmemsize()
{
    static uint64_t meminbytes = 0;
    if (meminbytes > 0) return meminbytes;

    for (size_t i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE) continue;

        meminbytes += mmap_tag->memmap[i].length;
    }

    return meminbytes;
}