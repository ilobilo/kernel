// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kernel::system::mm::heap {

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

extern bool initialised;

extern bool debug;

void init(void *heapAddr = (void*)0x0000100000000000, size_t pageCount = 0x10);

size_t getsize(void *ptr);

void *malloc(size_t size);
void *calloc(size_t m, size_t n);
void *realloc(void *ptr, size_t size);
void free(void *address);

void expandHeap(size_t length);
}

void *operator new(size_t size);
void *operator new[](size_t size);
void operator delete(void* ptr);