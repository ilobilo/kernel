#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/sched/lock/lock.hpp>
#include <system/mm/heap/heap.hpp>
#include <lib/string.hpp>

bool heap_initialised = false;

bool alloc_debug = true;

void *heapStart;
void *heapEnd;
heapSegHdr *lastHdr;

DEFINE_LOCK(heap_lock)

void Heap_init(void *heapAddr, size_t pageCount)
{
    serial_info("Initialising Kernel Heap\n");

    if (heap_initialised)
    {
        serial_info("Heap has already been initialised!\n");
        return;
    }

    if (!ptmanager_initialised)
    {
        serial_info("Page table manager has not been initialised!\n");
        PTManager_init();
    }

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

    heap_initialised = true;
}

void check_heap()
{
    if (!heap_initialised)
    {
        serial_info("Heap has not been initialised!\n");
        Heap_init();
    }
}

void free(void *address)
{
    check_heap();
    if (!address) return;

    acquire_lock(&heap_lock);
    heapSegHdr *segment = (heapSegHdr*)address - 1;
    segment->free = true;
    if (alloc_debug) serial_info("Free: Freeing %zu Bytes", segment->length);
    segment->combineForward();
    segment->combineBackward();
    release_lock(&heap_lock);
}

volatile bool expanded = false;
void* malloc(size_t size)
{
    check_heap();

    acquire_lock(&heap_lock);
    if (size > globalAlloc.getFreeRam())
    {
        serial_err("Malloc: requested more memory than available!");
        return NULL;
    }

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
                if (alloc_debug) serial_info("Malloc: Allocated %zu Bytes", size);
                expanded = false;
                release_lock(&heap_lock);
                return (void*)((uint64_t)currentSeg + sizeof(heapSegHdr));
            }
            if (currentSeg->length == size)
            {
                currentSeg->free = false;
                if (alloc_debug) serial_info("Malloc: Allocated %zu Bytes", size);
                expanded = false;
                release_lock(&heap_lock);
                return (void*)((uint64_t)currentSeg + sizeof(heapSegHdr));
            }
        }
        if (currentSeg->next == NULL) break;
        currentSeg = currentSeg->next;
    }
    if (expanded)
    {
        serial_err("Out of memory!");
        return NULL;
    }
    expandHeap(size);
    expanded = true;
    release_lock(&heap_lock);
    return malloc(size);
}

size_t alloc_getsize(void *ptr)
{
    heapSegHdr *segment = (heapSegHdr*)ptr - 1;
    return segment->length;
}

void *calloc(size_t num, size_t size)
{
    check_heap();

    void *ptr = malloc(num * size);
    if (!ptr) return NULL;

    memset(ptr, 0, num * size);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    check_heap();

    size_t oldsize = alloc_getsize(ptr);

    if (!size)
    {
        free(ptr);
        return NULL;
    }
    if (!ptr) return malloc(size);
    if (size <= oldsize) return ptr;

    void *newptr = malloc(size);
    if (!newptr) return ptr;

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

    if (alloc_debug) serial_info("Heap: Expanded heap with %zu Bytes", length);
}

void heapSegHdr::combineForward()
{
    if (next == NULL) return;
    if (!next->free) return;

    if (next == last) lastHdr = this;

    if (next->next != NULL) next->next->last = this;

    length = length + next->length + sizeof(heapSegHdr);

    next = next->next;
}

void heapSegHdr::combineBackward()
{
    if (last != NULL && last->free) last->combineForward();
}