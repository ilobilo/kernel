#include <main.hpp>
#include <stdint.h>

uint64_t getusablememsize()
{
    static uint64_t usablememinbytes = 0;
    if (usablememinbytes > 0) return usablememinbytes;

    for (int i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE && mmap_tag->memmap[i].type != STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE) continue;// && mmap_tag->memmap[i].type != STIVALE2_MMAP_KERNEL_AND_MODULES) continue;

        usablememinbytes += mmap_tag->memmap[i].length;
    }

    return usablememinbytes;
}


uint64_t getmemsize()
{
    static uint64_t meminbytes = 0;
    if (meminbytes > 0) return meminbytes;

    for (int i = 0; i < mmap_tag->entries; i++)
    {
        meminbytes += mmap_tag->memmap[i].length;
    }

    return meminbytes;
}