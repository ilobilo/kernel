// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <lib/memory.hpp>
#include <stivale2.h>
#include <main.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::mm::pfalloc {

Bitmap PageBitmap;
bool initialised = false;

uint64_t freeMem;
uint64_t reservedMem;
uint64_t usedMem;

uintptr_t highest_page = 0;

extern "C" uint64_t __kernelstart;
extern "C" uint64_t __kernelend;
void init()
{
    serial::info("Initialising Page Frame Allocator");

    if (initialised)
    {
        serial::info("Page frame allocator has already been initialised!\n");
        return;
    }

    for (size_t i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE) continue;

        uintptr_t top = mmap_tag->memmap[i].base;

        if (top > highest_page) highest_page = top;
    }

    uint64_t memsize = getmemsize();
    freeMem = memsize;
    uint64_t bitmapSize = memsize / 4096 / 8 + 1;

    Bitmap_init(bitmapSize, highest_page);

    reservePages(0, memsize / 4096 + 1);
    for (size_t i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE) continue;
        unreservePages((void*)mmap_tag->memmap[i].base, mmap_tag->memmap[i].length / 4096);
    }
    reservePages(0, 0x100);
    lockPages(PageBitmap.buffer, PageBitmap.size / 4096 + 1);

    uint64_t kernelsize = (uint64_t)&__kernelend - (uint64_t)&__kernelstart;
    uint64_t kernelpagecount = (uint64_t)kernelsize / 4096 + 1;

    pfalloc::lockPages((void*)&__kernelstart, kernelpagecount);

    serial::newline();
    initialised = true;
}

void Bitmap_init(size_t bitmapSize, uintptr_t bufferAddr)
{
    PageBitmap.size = bitmapSize;
    PageBitmap.buffer = (uint8_t*)bufferAddr;

    for (size_t i = 0; i < bitmapSize; i++)
    {
        *(uint8_t*)(PageBitmap.buffer + i) = 0;
    }
}

uint64_t pageBitmapIndex = 0;
void *requestPage()
{
    for (; pageBitmapIndex < PageBitmap.size * 8; pageBitmapIndex++)
    {
        if (PageBitmap[pageBitmapIndex] == true) continue;
        lockPage((void*)(pageBitmapIndex * 4096));
        return (void*)(pageBitmapIndex * 4096);
    }
    return NULL;
}

void *requestPages(uint64_t count)
{
    for (; pageBitmapIndex < PageBitmap.size * 8; pageBitmapIndex++)
    {
        for (uint64_t i = 0; i < count; i++)
        {
            if (PageBitmap[pageBitmapIndex + i] == true)
            {
                pageBitmapIndex += 1;
                continue;
            }
        }
        lockPages((void*)(pageBitmapIndex * 4096), count);
        return (void*)(pageBitmapIndex * 4096);
    }
    return NULL;
}

void freePage(void *address)
{
    uint64_t index = (uint64_t)address / 4096;
    if (PageBitmap[index] == false) return;
    if (PageBitmap.Set(index, false))
    {
        freeMem += 4096;
        usedMem -= 4096;
        if (pageBitmapIndex > index) pageBitmapIndex = index;
    }
}

void freePages(void *address, uint64_t pageCount)
{
    for (size_t t = 0; t < pageCount; t++)
    {
        freePage((void*)((uint64_t)address + (t * 4096)));
    }
}

void lockPage(void *address)
{
    uint64_t index = (uint64_t)address / 4096;
    if (PageBitmap[index] == true) return;
    if (PageBitmap.Set(index, true))
    {
        freeMem -= 4096;
        usedMem += 4096;
    }
}

void lockPages(void *address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        lockPage((void*)((uint64_t)address + (t * 4096)));
    }
}

void unreservePage(void *address)
{
    uint64_t index = (uint64_t)address / 4096;
    if (PageBitmap[index] == false) return;
    if (PageBitmap.Set(index, false))
    {
        freeMem += 4096;
        reservedMem-= 4096;
        if (pageBitmapIndex > index) pageBitmapIndex = index;
    }
}

void unreservePages(void *address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        unreservePage((void*)((uint64_t)address + (t * 4096)));
    }
}

void reservePage(void *address)
{
    uint64_t index = (uint64_t)address / 4096;
    if (PageBitmap[index] == true) return;
    if (PageBitmap.Set(index, true))
    {
        freeMem -= 4096;
        reservedMem += 4096;
    }
}

void reservePages(void *address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        reservePage((void*)((uint64_t)address + (t * 4096)));
    }
}

uint64_t getFreeRam()
{
    return freeMem;
}

uint64_t getUsedRam()
{
    return usedMem;
}

uint64_t getReservedRam()
{
    return reservedMem;
}
}