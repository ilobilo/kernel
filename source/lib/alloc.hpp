// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/liballoc.hpp>
#include <lib/buddy.hpp>
#include <lib/panic.hpp>
#include <lib/slab.hpp>
#include <cstddef>

enum allocs
{
    LIBALLOC,
    BUDDY,
    SLAB
};

static allocs defalloc = SLAB;

extern BuddyAlloc buddyheap;
extern SlabAlloc slabheap;

static inline void *malloc(size_t size, bool calloc = true)
{
    switch (defalloc)
    {
        case LIBALLOC:
            if (calloc) return liballoc_calloc(1, size);
            return liballoc_malloc(size);
            break;
        case BUDDY:
            if (calloc) return buddyheap.calloc(1, size);
            return buddyheap.malloc(size);
            break;
        case SLAB:
            if (calloc) return slabheap.calloc(1, size);
            return slabheap.malloc(size);
            break;
    }
    panic("No default allocator!");
    return nullptr;
}

static inline void *calloc(size_t num, size_t size)
{
    switch (defalloc)
    {
        case LIBALLOC:
            return liballoc_calloc(num, size);
            break;
        case BUDDY:
            return buddyheap.calloc(num, size);
            break;
        case SLAB:
            return slabheap.calloc(num, size);
            break;
    }
    panic("No default allocator!");
    return nullptr;
}

static inline void *realloc(void *ptr, size_t size)
{
    switch (defalloc)
    {
        case LIBALLOC:
            return liballoc_realloc(ptr, size);
            break;
        case BUDDY:
            return buddyheap.realloc(ptr, size);
            break;
        case SLAB:
            return slabheap.realloc(ptr, size);
            break;
    }
    panic("No default allocator!");
    return nullptr;
}

static inline void free(void *ptr)
{
    switch (defalloc)
    {
        case LIBALLOC:
            return liballoc_free(ptr);
            break;
        case BUDDY:
            return buddyheap.free(ptr);
            break;
        case SLAB:
            return slabheap.free(ptr);
            break;
    }
    panic("No default allocator!");
    return;
}

static inline size_t allocsize(void *ptr)
{
    switch (defalloc)
    {
        case LIBALLOC:
            return liballoc_allocsize(ptr);
            break;
        case BUDDY:
            return buddyheap.allocsize(ptr);
            break;
        case SLAB:
            return slabheap.allocsize(ptr);
            break;
    }
    panic("No default allocator!");
    return 0;
}

template<typename type>
static inline type malloc(size_t size)
{
    return reinterpret_cast<type>(malloc(size));
}

template<typename type>
static inline type calloc(size_t num, size_t size)
{
    return reinterpret_cast<type>(calloc(num, size));
}

template<typename type>
static inline type realloc(void *ptr, size_t size)
{
    return reinterpret_cast<type>(realloc(ptr, size));
}