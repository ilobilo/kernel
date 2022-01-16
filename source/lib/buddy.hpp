// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <stdint.h>
#include <stddef.h>

struct BuddyBlock
{
    size_t size;
    bool free;
};

class BuddyAlloc
{
    private:
    BuddyBlock *head = nullptr;
    BuddyBlock *tail = nullptr;
    void *data = nullptr;
    bool expanded = false;
    lock_t lock;

    BuddyBlock *next(BuddyBlock *block);
    BuddyBlock *split(BuddyBlock *block, size_t size);

    BuddyBlock *find_best(size_t size);
    size_t required_size(size_t size);
    void coalescence();

    public:
    bool debug = false;
    size_t pages = 0;

    void expand(size_t pagecount = 16);
    void setsize(size_t pagecount);

    void *malloc(size_t size);
    void *calloc(size_t num, size_t size);
    void *realloc(void *ptr, size_t size);
    void free(void *ptr);

    size_t allocsize(void *ptr);
};

extern BuddyAlloc kheap;

static inline void *malloc(size_t size, bool calloc = true)
{
    if (calloc) return kheap.calloc(1, size);
    return kheap.malloc(size);
}
static inline void *calloc(size_t num, size_t size)
{
    return kheap.calloc(num, size);
}
static inline void *realloc(void *ptr, size_t size)
{
    return kheap.realloc(ptr, size);
}
static inline void free(void *ptr)
{
    kheap.free(ptr);
}

static inline size_t allocsize(void *ptr)
{
    return kheap.allocsize(ptr);
}

void *operator new(size_t size);
void *operator new[](size_t size);

void operator delete(void *ptr);
void operator delete[](void *ptr);