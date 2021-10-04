#include <drivers/display/serial/serial.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/memory/memory.hpp>
#include <stivale2.h>
#include <main.hpp>

uint64_t freeMem;
uint64_t reservedMem;
uint64_t usedMem;
bool init = false;

PFAlloc globalAlloc;

uintptr_t highest_page = 0;

void PFAlloc::ReadMemMap()
{
    if (init) return;
    init = true;

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
}

void PFAlloc::Bitmap_init(size_t bitmapSize, uintptr_t bufferAddr)
{
    PageBitmap.size = bitmapSize;
    PageBitmap.buffer = (uint8_t*)bufferAddr;

    for (int i = 0; i < bitmapSize; i++)
    {
        *(uint8_t*)(PageBitmap.buffer + i) = 0;
    }
}

uint64_t pageBitmapIndex = 0;
void *PFAlloc::requestPage()
{
    for (; pageBitmapIndex < PageBitmap.size  *8; pageBitmapIndex++)
    {
        if (PageBitmap[pageBitmapIndex] == true) continue;
        lockPage((void*)(pageBitmapIndex  *4096));
        return (void*)(pageBitmapIndex  *4096);
    }

    return NULL; // Page frame swap
}

void PFAlloc::freePage(void *address)
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

void PFAlloc::freePages(void *address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        freePage((void*)((uint64_t)address + (t  *4096)));
    }
}

void PFAlloc::lockPage(void *address)
{
    uint64_t index = (uint64_t)address / 4096;
    if (PageBitmap[index] == true) return;
    if (PageBitmap.Set(index, true))
    {
        freeMem -= 4096;
        usedMem += 4096;
    }
}

void PFAlloc::lockPages(void *address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        lockPage((void*)((uint64_t)address + (t  *4096)));
    }
}

void PFAlloc::unreservePage(void *address)
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


void PFAlloc::unreservePages(void *address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        unreservePage((void*)((uint64_t)address + (t  *4096)));
    }
}

void PFAlloc::reservePage(void *address)
{
    uint64_t index = (uint64_t)address / 4096;
    if (PageBitmap[index] == true) return;
    if (PageBitmap.Set(index, true))
    {
        freeMem -= 4096;
        reservedMem += 4096;
    }
}

void PFAlloc::reservePages(void *address, uint64_t pageCount)
{
    for (int t = 0; t < pageCount; t++)
    {
        reservePage((void*)((uint64_t)address + (t  *4096)));
    }
}

uint64_t PFAlloc::getFreeRam()
{
    return freeMem;
}

uint64_t PFAlloc::getUsedRam()
{
    return usedMem;
}

uint64_t PFAlloc::getReservedRam()
{
    return reservedMem;
}

void PFAlloc_init()
{
    serial_info("Initializing Page Frame Allocator");

    globalAlloc = PFAlloc();
    globalAlloc.ReadMemMap();

    serial_info("Initialized Page Frame Allocator\n");
}