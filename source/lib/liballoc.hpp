#pragma once

#include <system/mm/pmm/pmm.hpp>
#include <lib/lock.hpp>
#include <stdint.h>

using namespace kernel::system::mm;

struct boundary_tag
{
    unsigned int magic;
    unsigned int size;
    unsigned int real_size;
    int index;

    boundary_tag *split_left;
    boundary_tag *split_right;

    boundary_tag *next;
    boundary_tag *prev;
};

DEFINE_LOCK(alloc_lock);

static inline int liballoc_lock()
{
    acquire_lock(alloc_lock);
    return 0;
}

static inline int liballoc_unlock()
{
    release_lock(alloc_lock);
    return 0;
}

static inline void *liballoc_alloc(int count)
{
    return pmm::alloc(count);
}

static inline int liballoc_free(void* ptr, int count)
{
    pmm::free(ptr, count);
    return 0;
}

void *malloc(size_t);
void *realloc(void *, size_t);
void *calloc(size_t, size_t);
void free(void *);
size_t allocsize(void *ptr);

void *operator new(size_t size);
void *operator new[](size_t size);

void operator delete(void *ptr);
void operator delete[](void *ptr);