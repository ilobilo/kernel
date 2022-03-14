// Copyright (C) 2021-2022  ilobilo

#include <system/mm/pmm/pmm.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/slab.hpp>
#include <lib/log.hpp>

using namespace kernel::system::mm;

void slab_t::init(uint64_t size)
{
    this->size = size;
    this->firstfree = reinterpret_cast<uint64_t>(pmm::alloc());

    uint64_t available = 0x1000 - ALIGN_UP(sizeof(slabHdr), this->size);
    slabHdr *slabptr = reinterpret_cast<slabHdr*>(this->firstfree);
    slabptr->slab = this;
    this->firstfree += ALIGN_UP(sizeof(slabHdr), this->size);

    uint64_t *array = reinterpret_cast<uint64_t*>(this->firstfree);
    uint64_t max = available / this->size - 1;
    uint64_t fact = this->size / 8;
    for (size_t i = 0; i < max; i++) array[i * fact] = reinterpret_cast<uint64_t>(&array[(i + 1) * fact]);
    array[max * fact] = 0;
}

void *slab_t::alloc()
{
    lockit(this->lock);

    if (this->firstfree == 0) this->init(this->size);
    uint64_t *oldfree = reinterpret_cast<uint64_t*>(this->firstfree);
    this->firstfree = oldfree[0];
    memset(oldfree, 0, this->size);
    return oldfree;
}

void slab_t::free(void *ptr)
{
    if (ptr == nullptr) return;
    lockit(this->lock);

    uint64_t *newhead = static_cast<uint64_t*>(ptr);
    newhead[0] = this->firstfree;
    this->firstfree = reinterpret_cast<uint64_t>(newhead);
}

SlabAlloc::SlabAlloc()
{
    this->slabs[0].init(8);
    this->slabs[1].init(16);
    this->slabs[2].init(24);
    this->slabs[3].init(32);
    this->slabs[4].init(48);
    this->slabs[5].init(64);
    this->slabs[6].init(128);
    this->slabs[7].init(256);
    this->slabs[8].init(512);
    this->slabs[9].init(1024);
}

slab_t *SlabAlloc::get_slab(size_t size)
{
    for (slab_t &slab : this->slabs)
    {
        if (slab.size >= size) return &slab;
    }
    return nullptr;
}

void *SlabAlloc::big_malloc(size_t size)
{
    size_t pages = DIV_ROUNDUP(size, 0x1000);
    void *ptr = pmm::alloc(pages + 1);
    if (ptr == nullptr) return nullptr;
    bigallocMeta *metadata = reinterpret_cast<bigallocMeta*>(ptr);
    metadata->pages = pages;
    metadata->size = size;
    return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(ptr) + 0x1000);
}

void *SlabAlloc::big_realloc(void *oldptr, size_t size)
{
    if (oldptr == nullptr) return this->malloc(size);

    bigallocMeta *metadata = reinterpret_cast<bigallocMeta*>(reinterpret_cast<uint64_t>(oldptr) - 0x1000);
    size_t oldsize = metadata->size;

    if (DIV_ROUNDUP(oldsize, 0x1000) == DIV_ROUNDUP(size, 0x1000))
    {
        metadata->size = size;
        return oldptr;
    }

    if (size == 0)
    {
        this->free(oldptr);
        return nullptr;
    }
    if (size < oldsize) oldsize = size;

    void *newptr = this->malloc(size);
    if (newptr == nullptr) return oldptr;

    memcpy(newptr, oldptr, oldsize);
    this->free(oldptr);
    return newptr;
}

void SlabAlloc::big_free(void *ptr)
{
    bigallocMeta *metadata = reinterpret_cast<bigallocMeta*>(reinterpret_cast<uint64_t>(ptr) - 0x1000);
    pmm::free(metadata, metadata->pages + 1);
}

size_t SlabAlloc::big_allocsize(void *ptr)
{
    return reinterpret_cast<bigallocMeta*>(reinterpret_cast<uint64_t>(ptr) - 0x1000)->size;
}

void *SlabAlloc::malloc(size_t size)
{
    slab_t *slab = this->get_slab(size);
    if (slab == nullptr) return this->big_malloc(size);
    return slab->alloc();
}

void *SlabAlloc::calloc(size_t num, size_t size)
{
    void *ptr = this->malloc(num * size);
    if (ptr == nullptr) return nullptr;

    memset(ptr, 0, num * size);
    return ptr;
}

void *SlabAlloc::realloc(void *oldptr, size_t size)
{
    if (oldptr == nullptr) return this->malloc(size);

    if ((reinterpret_cast<uint64_t>(oldptr) & 0xFFF) == 0) return this->big_realloc(oldptr, size);

    slab_t *slab = reinterpret_cast<slabHdr*>(reinterpret_cast<uint64_t>(oldptr) & ~0xFFF)->slab;
    size_t oldsize = slab->size;

    if (size == 0)
    {
        this->free(oldptr);
        return nullptr;
    }
    if (size < oldsize) oldsize = size;

    void *newptr = this->malloc(size);
    if (newptr == nullptr) return oldptr;

    memcpy(newptr, oldptr, oldsize);
    this->free(oldptr);
    return newptr;
}

void SlabAlloc::free(void *ptr)
{
    if (ptr == nullptr) return;

    if ((reinterpret_cast<uint64_t>(ptr) & 0xFFF) == 0) return this->big_free(ptr);
    reinterpret_cast<slabHdr*>(reinterpret_cast<uint64_t>(ptr) & ~0xFFF)->slab->free(ptr);
}

size_t SlabAlloc::allocsize(void *ptr)
{
    if (ptr == nullptr) return 0;

    if ((reinterpret_cast<uint64_t>(ptr) & 0xFFF) == 0) return this->big_allocsize(ptr);
    return reinterpret_cast<slabHdr*>(reinterpret_cast<uint64_t>(ptr) & ~0xFFF)->slab->size;
}