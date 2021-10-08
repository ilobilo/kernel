#include <main.hpp>
#include <stdint.h>

uint64_t getmemsize()
{
    static uint64_t meminbytes = 0;
    if (meminbytes > 0) return meminbytes;

    for (int i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE) continue;

        meminbytes += mmap_tag->memmap[i].length;
    }

    return meminbytes;
}
