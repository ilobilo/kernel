// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/liballoc.hpp>
#include <lib/buddy.hpp>
#include <lib/slab.hpp>
#include <stddef.h>

#define LIBALLOC 0
#define BUDDY 1
#define SLAB 2

#define ALLOC_IMPL SLAB

#if (ALLOC_IMPL == BUDDY)
extern BuddyAlloc buddyheap;
#elif (ALLOC_IMPL == SLAB)
extern SlabAlloc slabheap;
#endif

static inline void *malloc(size_t size, bool calloc = true)
{
#if (ALLOC_IMPL == LIBALLOC)

    if (calloc) return liballoc_calloc(1, size);
    return liballoc_malloc(size);

#elif (ALLOC_IMPL == BUDDY)

    if (calloc) return buddyheap.calloc(1, size);
    return buddyheap.malloc(size);

#elif (ALLOC_IMPL == SLAB)

    if (calloc) return slabheap.calloc(1, size);
    return slabheap.malloc(size);

#else
    return nullptr;
#endif
}

static inline void *calloc(size_t num, size_t size)
{
#if (ALLOC_IMPL == LIBALLOC)

    return liballoc_calloc(num, size);

#elif (ALLOC_IMPL == BUDDY)

    return buddyheap.calloc(num, size);

#elif (ALLOC_IMPL == SLAB)

    return slabheap.calloc(num, size);

#else
    return nullptr;
#endif
}

static inline void *realloc(void *ptr, size_t size)
{
#if (ALLOC_IMPL == LIBALLOC)

    return liballoc_realloc(ptr, size);

#elif (ALLOC_IMPL == BUDDY)

    return buddyheap.realloc(ptr, size);

#elif (ALLOC_IMPL == SLAB)

    return slabheap.realloc(ptr, size);

#else
    return nullptr;
#endif
}

static inline void free(void *ptr)
{
#if (ALLOC_IMPL == LIBALLOC)

    liballoc_free(ptr);

#elif (ALLOC_IMPL == BUDDY)

    buddyheap.free(ptr);

#elif (ALLOC_IMPL == SLAB)

    slabheap.free(ptr);

#endif
}

static inline size_t allocsize(void *ptr)
{
#if (ALLOC_IMPL == LIBALLOC)

    return liballoc_allocsize(ptr);

#elif (ALLOC_IMPL == BUDDY)

    return buddyheap.allocsize(ptr);

#elif (ALLOC_IMPL == SLAB)

    return slabheap.allocsize(ptr);

#else
    return 0;
#endif
}