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
    public:
    BuddyBlock *head;
    BuddyBlock *tail;
    void *data = nullptr;

    volatile lock_t lock;
    volatile bool expanded = false;

    BuddyBlock *next(BuddyBlock *block);
    BuddyBlock *split(BuddyBlock *block, size_t size);

    BuddyBlock *find_best(size_t size);
    size_t required_size(size_t size);
    void coalescence();

    public:
    bool debug = false;
    size_t pages = 0;

    BuddyAlloc(size_t pagecount);
    ~BuddyAlloc();

    void expand(size_t pagecount);
    void setsize(size_t pagecount);

    void *malloc(size_t size);
    void *calloc(size_t num, size_t size);
    void *realloc(void *ptr, size_t size);
    void free(void *ptr);

    size_t allocsize(void *ptr);
};

extern BuddyAlloc *heap;