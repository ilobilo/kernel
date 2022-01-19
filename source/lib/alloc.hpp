// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/liballoc.hpp>
#include <lib/buddy.hpp>
#include <stddef.h>

#define LIBALLOC 0
#define BUDDY 1
#define ALLOC_IMPL BUDDY

static inline void *malloc(size_t size, bool calloc = true)
{
#if (ALLOC_IMPL == LIBALLOC)
    if (calloc) return liballoc_calloc(1, size);
    return liballoc_malloc(size);
#elif (ALLOC_IMPL == BUDDY)
    if (calloc) return kheap.calloc(1, size);
    return kheap.malloc(size);
#else
    return nullptr;
#endif
}
static inline void *calloc(size_t num, size_t size)
{
#if (ALLOC_IMPL == LIBALLOC)
    return liballoc_calloc(num, size);
#elif (ALLOC_IMPL == BUDDY)
    return kheap.calloc(num, size);
#else
    return nullptr;
#endif
}
static inline void *realloc(void *ptr, size_t size)
{
#if (ALLOC_IMPL == LIBALLOC)
    return liballoc_realloc(ptr, size);
#elif (ALLOC_IMPL == BUDDY)
    return kheap.realloc(ptr, size);
#else
    return nullptr;
#endif
}
static inline void free(void *ptr)
{
#if (ALLOC_IMPL == LIBALLOC)
    liballoc_free(ptr);
#elif (ALLOC_IMPL == BUDDY)
    kheap.free(ptr);
#endif
}
static inline size_t allocsize(void *ptr)
{
#if (ALLOC_IMPL == LIBALLOC)
    return liballoc_allocsize(ptr);
#elif (ALLOC_IMPL == BUDDY)
    return kheap.allocsize(ptr);
#else
    return 0;
#endif
}

void *operator new(size_t size);
void *operator new[](size_t size);

void operator delete(void *ptr);
void operator delete[](void *ptr);