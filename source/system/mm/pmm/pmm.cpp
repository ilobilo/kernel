// Copyright (C) 2021-2022  ilobilo

#include <system/mm/pmm/pmm.hpp>
#include <kernel/kernel.hpp>
#include <lib/memory.hpp>
#include <lib/panic.hpp>
#include <lib/math.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>

namespace kernel::system::mm::pmm {

Bitmap bitmap;
bool initialised = false;
static uintptr_t highest_addr = 0;
static size_t lastI = 0;
static size_t usedRam = 0;
static size_t freeRam = 0;

new_lock(pmm_lock);

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
    lockit(pmm_lock);

    size_t i = lastI;
    void *ret = inner_alloc(count, highest_addr / 0x1000);
    if (ret == nullptr)
    {
        lastI = 0;
        ret = inner_alloc(count, i);
        if (ret == nullptr) panic("Out of memory!");
    }
    memset(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(ret) + hhdm_offset), 0, count * 0x1000);

    usedRam += count * 0x1000;
    freeRam -= count * 0x1000;

    return ret;
}

void free(void *ptr, size_t count)
{
    if (ptr == nullptr) return;
    lockit(pmm_lock);

    size_t page = reinterpret_cast<size_t>(ptr) / 0x1000;
    for (size_t i = page; i < page + count; i++) bitmap.Set(i, false);
    if (lastI > page) lastI = page;

    usedRam -= count * 0x1000;
    freeRam += count * 0x1000;
}

void *realloc(void *ptr, size_t oldcount, size_t newcount)
{
    if (ptr == nullptr) return alloc(newcount);

    if (!newcount)
    {
        free(ptr, oldcount);
        return nullptr;
    }

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
    log("Initialising PMM");

    if (initialised)
    {
        warn("PMM has already been initialised!\n");
        return;
    }

    limine_memmap_entry **memmaps = memmap_request.response->entries;
    uint64_t memmap_count = memmap_request.response->entry_count;

    for (size_t i = 0; i < memmap_count; i++)
    {
        if (memmaps[i]->type != LIMINE_MEMMAP_USABLE) continue;

        uintptr_t top = memmaps[i]->base + memmaps[i]->length;
        freeRam += memmaps[i]->length;

        if (top > highest_addr) highest_addr = top;
    }

    size_t bitmapSize = ALIGN_UP((highest_addr / 0x1000) / 8, 0x1000);

    for (size_t i = 0; i < memmap_count; i++)
    {
        if (memmaps[i]->type != LIMINE_MEMMAP_USABLE) continue;

        if (memmaps[i]->length >= bitmapSize)
        {
            bitmap.buffer = reinterpret_cast<uint8_t*>(memmaps[i]->base);
            memset(bitmap.buffer, 0xFF, bitmapSize);

            memmaps[i]->length -= bitmapSize;
            memmaps[i]->base += bitmapSize;
            freeRam -= bitmapSize;
            break;
        }
    }

    for (size_t i = 0; i < memmap_count; i++)
    {
        if (memmaps[i]->type != LIMINE_MEMMAP_USABLE) continue;

        for (uintptr_t t = 0; t < memmaps[i]->length; t += 0x1000)
        {
            bitmap.Set((memmaps[i]->base + t) / 0x1000, false);
        }
    }

    serial::newline();
    initialised = true;
}
}