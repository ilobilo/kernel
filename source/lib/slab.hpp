// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <stdint.h>
#include <stddef.h>

struct slab_t
{
    lock_t lock;
    uint64_t firstfree;
    uint64_t size;

    void init(uint64_t size);
    void *alloc();
    void free(void *ptr);
};

struct slabHdr
{
    slab_t *slab;
};

class SlabAlloc
{
    private:
    lock_t lock;

    struct bigallocMeta
    {
        size_t pages;
        size_t size;
    };

    slab_t *get_slab(size_t size);

    void *big_malloc(size_t size);
    void *big_realloc(void *oldptr, size_t size);
    void big_free(void *ptr);
    size_t big_allocsize(void *ptr);

    public:
    slab_t slabs[10];

    SlabAlloc();

    void *malloc(size_t size);
    void *calloc(size_t num, size_t size);
    void *realloc(void *oldptr, size_t size);
    void free(void *ptr);
    size_t allocsize(void *ptr);
};