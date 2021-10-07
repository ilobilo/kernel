#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/heap/heap.hpp>
#include <lib/string.hpp>

void *heapStart;
void *heapEnd;
heapSegHdr *lastHdr;

void Heap_init(void *heapAddr, size_t pageCount)
{
    serial_info("Initialising Kernel Heap\n");

    void *pos = heapAddr;
    for (size_t i = 0; i < pageCount; i++)
    {
        globalPTManager.mapMem(pos, globalAlloc.requestPage());
        pos = (void*)((size_t)pos + 0x1000);
    }
    size_t heapLength = pageCount * 0x1000;

    heapStart = heapAddr;
    heapEnd = (void*)((size_t)heapStart + heapLength);
    heapSegHdr *startSeg = (heapSegHdr*)heapAddr;

    startSeg->length = heapLength - sizeof(heapSegHdr);
    startSeg->next = NULL;
    startSeg->last = NULL;
    startSeg->free = true;
    lastHdr = startSeg;
}

void free(void *address)
{
    heapSegHdr *segment = (heapSegHdr*)address - 1;
    segment->free = true;
    serial_info("Free: Freeing %zu Bytes", segment->length);
    segment->combineForward();
    segment->combineBackward();
}

void* malloc(size_t size)
{
    if (size % 0x08 > 0)
    {
        size -= (size % 0x08);
        size += 0x08;
    }
    if (size == 0) return NULL;

    heapSegHdr* currentSeg = (heapSegHdr*)heapStart;
    while (true)
    {
        if (currentSeg->free)
        {
            if (currentSeg->length > size)
            {
                currentSeg->split(size);
                currentSeg->free = false;
                serial_info("Malloc: Allocated %zu Bytes", size);
                return (void*)((uint64_t)currentSeg + sizeof(heapSegHdr));
            }
            if (currentSeg->length == size)
            {
                currentSeg->free = false;
                serial_info("Malloc: Allocated %zu Bytes", size);
                return (void*)((uint64_t)currentSeg + sizeof(heapSegHdr));
            }
        }
        if (currentSeg->next == NULL) break;
        currentSeg = currentSeg->next;
    }
    volatile bool expanded = false;
    if (expanded)
    {
        serial_err("Out of memory!");
        return NULL;
    }
    expandHeap(size);
    expanded = true;
    return malloc(size);
}

size_t alloc_getsize(void *ptr)
{
    heapSegHdr *segment = (heapSegHdr*)ptr - 1;
    return segment->length;
}

void *calloc(size_t m, size_t n)
{
    void *p;
    size_t *z;
    if (n && m > (size_t)-1 / n) return NULL;
    n *= m;
    p = malloc(n);
    if (!p) return NULL;
    memset(p, 0, n);
    return p;
}

void *realloc(void *ptr, size_t size)
{
    size_t oldsize = alloc_getsize(ptr);
    void *newptr;

    if (ptr == NULL) return malloc(size);
    if (size <= oldsize) return ptr;

    newptr = malloc(size);
    memcpy(newptr, ptr, oldsize);
    free(ptr);
    return(newptr);
}

heapSegHdr *heapSegHdr::split(size_t splitLength)
{
    if (splitLength < 0x08) return NULL;
    int64_t splitSegLength = length - splitLength - sizeof(heapSegHdr);
    if (splitSegLength < 0x08) return NULL;

    heapSegHdr *newSplitHdr = (heapSegHdr*)((size_t)this + splitLength + sizeof(heapSegHdr));

    next->last = newSplitHdr;
    newSplitHdr->next = next;
    next = newSplitHdr;
    newSplitHdr->last = this;
    newSplitHdr->length = splitSegLength;
    newSplitHdr->free = free;
    length = splitLength;

    if (lastHdr == this) lastHdr = newSplitHdr;
    return newSplitHdr;
}

void expandHeap(size_t length)
{
    if (length % 0x1000)
    {
        length -= length % 0x1000;
        length += 0x1000;
    }
    size_t pageCount = length / 0x1000;
    heapSegHdr *newSeg = (heapSegHdr*)heapEnd;

    for (size_t i = 0; i < pageCount; i++)
    {
        globalPTManager.mapMem(heapEnd, globalAlloc.requestPage());
        heapEnd = (void*)((size_t)heapEnd + 0x1000);
    }

    newSeg->free = true;
    newSeg->last = lastHdr;
    lastHdr->next = newSeg;
    lastHdr = newSeg;
    newSeg->next = NULL;
    newSeg->length = length - sizeof(heapSegHdr);
    newSeg->combineBackward();

    serial_info("Heap: Expanded heap with %zu Bytes", length);
}

void heapSegHdr::combineForward()
{
    if (next == NULL) return;
    if (!next->free) return;

    if (next == last) lastHdr = this;

    if (next->next != NULL) next->next->last = this;

    length = length + next->length + sizeof(heapSegHdr);
}

void heapSegHdr::combineBackward()
{
    if (last != NULL && last->free) last->combineForward();
}
