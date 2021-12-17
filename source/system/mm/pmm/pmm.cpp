// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <kernel/main.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/lock.hpp>
#include <stivale2.h>

using namespace kernel::drivers::display;

namespace kernel::system::mm::pmm {

Bitmap bitmap;
bool initialised = false;
static uintptr_t highest_page = 0;
static size_t lastI = 0;
static size_t usedRam = 0;
static size_t freeRam = 0;

DEFINE_LOCK(pmm_lock)

static void *inner_alloc(size_t count, size_t limit)
{
    size_t p = 0;

    while (lastI < limit)
    {
        if (!bitmap[lastI++])
        {
            if (++p == count)
            {
                size_t page = lastI - count;
                for (size_t i = page; i < lastI; i++) bitmap.Set(i, true);
                return reinterpret_cast<void*>(page * 0x1000);
            }
        }
        else p = 0;
    }
    return nullptr;
}

void *alloc(size_t count)
{
    acquire_lock(pmm_lock);
    size_t l = lastI;
    void *ret = inner_alloc(count, highest_page / 0x1000);
    if (!ret)
    {
        lastI = 0;
        ret = inner_alloc(count, 1);
    }
    memset(ret, 0, count * 0x1000);
    usedRam += count * 0x1000;
    freeRam -= count * 0x1000;
    release_lock(pmm_lock);
    return ret;
}

void free(void *ptr, size_t count)
{
    if (!ptr) return;
    acquire_lock(pmm_lock);
    size_t page = reinterpret_cast<size_t>(ptr) / 0x1000;
    for (size_t i = page; i < page + count; i++) bitmap.Set(i, false);
    usedRam -= count * 0x1000;
    freeRam += count * 0x1000;
    release_lock(pmm_lock);
}

void *realloc(void *ptr, size_t oldcount, size_t newcount)
{
    if (!ptr) return alloc(newcount);

    if (!newcount)
    {
        free(ptr, oldcount);
        return nullptr;
    }

    usedRam = usedRam - oldcount * 0x1000 + newcount * 0x1000;
    freeRam = freeRam + oldcount * 0x1000 - newcount * 0x1000;

    if (newcount < oldcount) oldcount = newcount;

    void *newptr = alloc(newcount);
    memcpy(newptr, ptr, oldcount);
    free(ptr);
    return newptr;
}

size_t freemem()
{
    return freeRam;
}

size_t usedmem()
{
    return usedRam;
}

void init()
{
    serial::info("Initialising PMM");

    if (initialised)
    {
        serial::warn("PMM has already been initialised!\n");
        return;
    }

    for (size_t i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE) continue;

        uintptr_t top = mmap_tag->memmap[i].base + mmap_tag->memmap[i].length;

        if (top > highest_page) highest_page = top;
    }

    size_t bitmapSize = ALIGN_UP((highest_page / 0x1000) / 8, 0x1000);

    for (size_t i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE) continue;

        if (mmap_tag->memmap[i].length >= bitmapSize)
        {
            bitmap.buffer = reinterpret_cast<uint8_t*>(mmap_tag->memmap[i].base);
            memset(bitmap.buffer, 0xFF, bitmapSize);

            mmap_tag->memmap[i].length -= bitmapSize;
            mmap_tag->memmap[i].base += bitmapSize;
            break;
        }
    }

    for (size_t i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE) continue;

        for (uintptr_t t = 0; t < mmap_tag->memmap[i].length; t += 0x1000)
        {
            bitmap.Set((mmap_tag->memmap[i].base + t) / 0x1000, false);
        }
    }

    freeRam = getmemsize();

    serial::newline();
    initialised = true;
}
}