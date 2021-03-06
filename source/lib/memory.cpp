// Copyright (C) 2021-2022  ilobilo

#include <kernel/kernel.hpp>
#include <cstdint>
#include <cstddef>

uint64_t getmemsize()
{
    static uint64_t meminbytes = 0;
    if (meminbytes > 0) return meminbytes;

    for (size_t i = 0; i < memmap_request.response->entry_count; i++)
    {
        if (memmap_request.response->entries[i]->type != LIMINE_MEMMAP_USABLE) continue;

        meminbytes += memmap_request.response->entries[i]->length;
    }

    return meminbytes;
}

bool arraycmp(uint8_t *a1, uint8_t *a2, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (a1[i] != a2[i]) return false;
    }
    return true;
}