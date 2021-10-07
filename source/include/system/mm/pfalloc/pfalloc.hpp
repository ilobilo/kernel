#pragma once

#include <system/mm/bitmap/bitmap.hpp>
#include <stdint.h>
#include <main.hpp>

class PFAlloc
{
    public:
    void ReadMemMap();
    Bitmap PageBitmap;
    
    void freePage(void *address);
    void lockPage(void *address);
    void freePages(void *address, uint64_t pageCount);
    void lockPages(void *address, uint64_t pageCount);

    void *requestPage();

    uint64_t getFreeRam();
    uint64_t getUsedRam();
    uint64_t getReservedRam();

    private:
    void Bitmap_init(size_t bitmapSize, uintptr_t bufferAddr);
    void reservePage(void *address);
    void unreservePage(void *address);
    void reservePages(void *address, uint64_t pageCount);
    void unreservePages(void *address, uint64_t pageCount);
};

extern PFAlloc globalAlloc;

extern bool pfalloc_initialised;

void PFAlloc_init();