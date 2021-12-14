// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <lib/memory.hpp>
#include <lib/lock.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::mm::heap {

bool initialised = false;
bool debug = false;

heapSegHdr *lastSeg;
heapSegHdr *mainSeg;

size_t totalSize;
size_t freeSize;
size_t usedSize;

void *heapEnd;

DEFINE_LOCK(heap_lock)

void init(void *heapAddr, size_t pageCount)
{
    serial::info("Initialising Heap");

    if (initialised)
    {
        serial::warn("Heap has already been initialised!\n");
        return;
    }

    heapEnd = heapAddr;
    expandHeap(pageCount * 0x1000);

    serial::newline();
    initialised = true;
}

void check()
{
    if (!initialised)
    {
        serial::err("Heap has not been initialised!");
        init();
    }
}

void free(void *address)
{
    check();
    if (!address) return;

    acquire_lock(heap_lock);
    heapSegHdr *header = (heapSegHdr*)((uint64_t)address - sizeof(heapSegHdr));
    header->free = true;
    freeSize += header->length + sizeof(heapSegHdr);
    usedSize -= header->length + sizeof(heapSegHdr);

    if (debug) serial::info("Free: Freeing %zu Bytes", header->length + sizeof(heapSegHdr));

    if (header->next && header->last)
    {
        if (header->next->free && header->last->free)
        {
            header->mergeNextAndThisToLast();
        }
    }
    else if (header->last)
    {
        if (header->last->free)
        {
            header->mergeThisToLast();
        }
    }
    else if (header->next)
    {
        if (header->next->free)
        {
            header->mergeNextToThis();
        }
    }
    release_lock(heap_lock);
}

static volatile bool expanded = false;
void* malloc(size_t size)
{
    check();

    acquire_lock(heap_lock);

    if (size % blockSize)
    {
        size -= (size % blockSize);
        size += blockSize;
    }
    if (!size) return NULL;

    heapSegHdr* currentSeg = (heapSegHdr*)mainSeg;
    while (true)
    {
        if (currentSeg->free)
        {
            if (currentSeg->length > size)
            {
                currentSeg->split(size);
                currentSeg->free = false;
                usedSize += currentSeg->length + sizeof(heapSegHdr);
                freeSize -= currentSeg->length + sizeof(heapSegHdr);
                if (debug) serial::info("Malloc: Allocated %zu Bytes", size + sizeof(heapSegHdr));

                expanded = false;
                release_lock(heap_lock);
                return (void*)((uint64_t)currentSeg + sizeof(heapSegHdr));
            }
            if (currentSeg->length == size)
            {
                currentSeg->free = false;
                usedSize += currentSeg->length + sizeof(heapSegHdr);
                freeSize -= currentSeg->length + sizeof(heapSegHdr);
                if (debug) serial::info("Malloc: Allocated %zu Bytes", size + sizeof(heapSegHdr));

                expanded = false;
                release_lock(heap_lock);
                return (void*)((uint64_t)currentSeg + sizeof(heapSegHdr));
            }
        }
        if (!currentSeg->next) break;
        currentSeg = currentSeg->next;
    }
    if (expanded)
    {
        serial::err("Could not expand the heap!");
        return NULL;
    }
    expandHeap(size);
    expanded = true;
    release_lock(heap_lock);
    return malloc(size);
}

void heapSegHdr::mergeNextAndThisToLast()
{
    if (this->next == lastSeg)
    {
        if (this->next->next) lastSeg = this->next->next;
        else lastSeg = this->last;
    }
    if (this->next == mainSeg)
    {
        if (this->next->last) mainSeg = this->next->last;
        else mainSeg = this->next->next;
    }
    if (this == lastSeg)
    {
        if (this->next->next) lastSeg = this->next->next;
        else lastSeg = this->last;
    }
    if (this == mainSeg)
    {
        if (this->last) mainSeg = this->last;
        else mainSeg = this->next->next;
    }

    this->last->length += this->length + sizeof(heapSegHdr) + this->next->length + sizeof(heapSegHdr);
    this->last->next = this->next->next;
    this->next->next->last = this->last;

    memset(this->next, 0, sizeof(heapSegHdr));
    memset(this, 0, sizeof(heapSegHdr));
}

void heapSegHdr::mergeThisToLast()
{
    this->last->length += this->length + sizeof(heapSegHdr);
    this->last->next = this->next;
    this->next->last = this->last;
    if (this == lastSeg)
    {
        if (this->next) lastSeg = this->next;
        else lastSeg = this->last;
    }
    if (this == mainSeg)
    {
        if (this->last) mainSeg = this->last;
        else mainSeg = this->next;
    }
    memset(this, 0, sizeof(heapSegHdr));
}

void heapSegHdr::mergeNextToThis()
{
    heapSegHdr *nextHdr = this->next;
    this->length += this->next->length + sizeof(heapSegHdr);
    this->next = this->next->next;
    this->next->last = this;
    
    if (this == lastSeg)
    {
        if (this->next->next) lastSeg = this->next->next;
        else lastSeg = this;
    }
    if (nextHdr == lastSeg)
    {
        if (this->next) lastSeg = this->next;
        else lastSeg = this;
    }
    if (this ==  mainSeg) mainSeg = this->last;

    memset(nextHdr, 0, sizeof(heapSegHdr));
}

size_t getsize(void *ptr)
{
    heapSegHdr *segment = (heapSegHdr*)ptr - 1;
    return segment->length;
}

void *calloc(size_t num, size_t size)
{
    check();

    void *ptr = malloc(num * size);
    if (!ptr) return NULL;

    memset(ptr, 0, num * size);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    check();

    if (!ptr) return malloc(size);

    size_t oldsize = heap::getsize(ptr);

    if (!size)
    {
        free(ptr);
        return NULL;
    }
    if (size < oldsize) oldsize = size;

    void *newptr = malloc(size);
    if (!newptr) return ptr;

    memcpy(newptr, ptr, oldsize);
    free(ptr);
    return(newptr);
}

void heapSegHdr::split(size_t size)
{
    if (this->length < size + sizeof(heapSegHdr)) return;

    heapSegHdr *newSeg = (heapSegHdr*)((uint64_t)this + sizeof(heapSegHdr) + size);
    memset(newSeg, 0, sizeof(heapSegHdr));
    newSeg->free = true;
    newSeg->length = this->length - size - sizeof(heapSegHdr);
    newSeg->next = this->next;
    newSeg->last = this;

    if (!this->next) lastSeg = newSeg;
    this->next = newSeg;
    this->length = size;
}

void expandHeap(size_t length)
{
    length += sizeof(heapSegHdr);
    if (length % 0x1000)
    {
        length -= length % 0x1000;
        length += 0x1000;
    }
    size_t pageCount = length / 0x1000;
    heapSegHdr *newSeg = (heapSegHdr*)heapEnd;

    for (size_t i = 0; i < pageCount; i++)
    {
        uint64_t t = (uint64_t)pmm::alloc();
        if (!t) serial::err("FSF");
        vmm::kernel_pagemap->mapMem((uint64_t)heapEnd, t);
        heapEnd = (void*)((size_t)heapEnd + 0x1000);
    }

    if (lastSeg && lastSeg->free) lastSeg->length += length;
    else
    {
        newSeg->length = length - sizeof(heapSegHdr);
        newSeg->free = true;
        newSeg->last = lastSeg;
        newSeg->next = NULL;
        if (lastSeg) lastSeg->next = newSeg;
        lastSeg = newSeg;
    }

    if (!mainSeg) mainSeg = newSeg;
    totalSize += length + sizeof(heapSegHdr);
    freeSize -= length + sizeof(heapSegHdr);

    if (debug) serial::info("Heap: Expanded heap with %zu Bytes", length);
}
}

void *operator new(size_t size)
{
    return kernel::system::mm::heap::malloc(size);
}

void *operator new[](size_t size)
{
    return kernel::system::mm::heap::malloc(size);
}

void operator delete(void* ptr)
{
    kernel::system::mm::heap::free(ptr);
}