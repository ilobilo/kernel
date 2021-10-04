#pragma once

#include <stdint.h>
#include <stddef.h>

struct heapSegHdr
{
    size_t length;
    heapSegHdr *next;
    heapSegHdr *last;
    bool free;
    void combineForward();
    void combineBackward();
    heapSegHdr *split(size_t splitLength);
};

void Heap_init(void *heapAddr, size_t pageCount);

void *malloc(size_t size);

size_t alloc_getsize(void *ptr);

void *calloc(size_t m, size_t n);

void *realloc(void *ptr, size_t size);

void free(void *address);

void expandHeap(size_t length);
