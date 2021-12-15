// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <lib/memory.hpp>
#include <lib/panic.hpp>
#include <lib/buddy.hpp>
#include <lib/math.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

BuddyAlloc *heap;

BuddyBlock *BuddyAlloc::next(BuddyBlock *block)
{
    return (BuddyBlock*)((char*)block + block->size);
}

BuddyBlock *BuddyAlloc::split(BuddyBlock *block, size_t size)
{
    if (block != NULL && size != 0)
    {
        while (size < block->size)
        {
            size_t sz = block->size >> 1;
            block->size = sz;
            block = this->next(block);
            block->size = sz;
            block->free = true;
        }
        if (size <= block->size) return block;
    }
    return NULL;
}

BuddyBlock *BuddyAlloc::find_best(size_t size)
{
    if (size == 0) return NULL;

    BuddyBlock *best_block = NULL;
    BuddyBlock *block = this->head;
    BuddyBlock *buddy = this->next(block);

    if (buddy == this->tail && block->free) return this->split(block, size);

    while (block < this->tail && buddy < this->tail)
    {
        if (block->free && buddy->free && block->size == buddy->size)
        {
            block->size <<= 1;
            if (size <= block->size && (best_block == NULL || block->size <= best_block->size)) best_block = block;

            block = this->next(buddy);
            if (block < this->tail) buddy = this->next(block);
            continue;
        }

        if (block->free && size <= block->size && (best_block == NULL || block->size <= best_block->size)) best_block = block;
        if (buddy->free && size <= buddy->size && (best_block == NULL || buddy->size < best_block->size)) best_block = buddy;

        if (block->size <= buddy->size)
        {
            block = this->next(buddy);
            if (block < this->tail) buddy = this->next(block);
        }
        else
        {
            block = buddy;
            buddy = this->next(buddy);
        }
    }

    if (best_block != NULL) return this->split(best_block, size);

    return NULL;
}

size_t BuddyAlloc::required_size(size_t size)
{
    size_t actual_size = sizeof(BuddyBlock);

    size += sizeof(BuddyBlock);
    size = ALIGN_UP(size, sizeof(BuddyBlock));

    while (size > actual_size) actual_size <<= 1;

    return actual_size;
}

void BuddyAlloc::coalescence()
{
    while (true)
    {
        BuddyBlock *block = this->head;
        BuddyBlock *buddy = this->next(block);

        bool no_coalescence = true;
        while (block < this->tail && buddy < this->tail)
        {
            if (block->free && buddy->free && block->size == buddy->size)
            {
                block->size <<= 1;
                block = this->next(block);
                if (block < this->tail)
                {
                    buddy = this->next(block);
                    no_coalescence = false;
                }
            }
            else if (block->size < buddy->size)
            {
                block = buddy;
                buddy = this->next(buddy);
            }
            else
            {
                block = this->next(buddy);
                if (block < this->tail) buddy = this->next(block);
            }
        }

        if (no_coalescence) return;
    }
}

BuddyAlloc::BuddyAlloc(size_t pagecount)
{
    expand(pagecount);
}

BuddyAlloc::~BuddyAlloc()
{
    this->head = nullptr;
    this->tail = nullptr;
    pmm::free(this->data);
}

void BuddyAlloc::expand(size_t pagecount)
{
    acquire_lock(this->lock);
    ASSERT(pagecount != 0, "Page count can not be zero!");
    size_t size = (pagecount + this->pages) * 0x1000;
    while (size % 0x1000 || !POWER_OF_2(size))
    {
        size++;
        size = to_power_of_2(size);
    }
    pagecount = size / 0x1000;
    ASSERT(POWER_OF_2(size), "Size is not power of two!");

    this->data = pmm::realloc(this->data, pagecount);
    ASSERT(data != NULL, "Could not allocate memory!");

    this->head = (BuddyBlock*)data;
    this->head->size = size;
    this->head->free = true;

    this->tail = next(head);
    this->pages = pagecount;

    if (this->debug) serial::info("Expanded the heap. Current size: %zu bytes, %zu pages", size, pagecount);
    release_lock(this->lock);
}

void BuddyAlloc::setsize(size_t pagecount)
{
    ASSERT(pagecount != 0, "Page count can not be zero!");
    ASSERT(pagecount > this->pages, "Page count needs to be higher than current size!");
    pagecount = pagecount - this->pages;
    this->expand(pagecount);
}

void *BuddyAlloc::malloc(size_t size)
{
    if (size == 0) return NULL;

    acquire_lock(this->lock);

    size_t actual_size = this->required_size(size);

    BuddyBlock *found = this->find_best(actual_size);
    if (found == NULL)
    {
        this->coalescence();
        found = this->find_best(actual_size);
    }

    if (found != NULL)
    {
        if (this->debug) serial::info("Allocated %zu bytes");
        found->free = false;
        this->expanded = false;
        release_lock(this->lock);
        return (void*)((char*)found + sizeof(BuddyBlock));
    }

    if (this->expanded)
    {
        if (this->debug) serial::err("Could not expand the heap!");
        this->expanded = false;
        release_lock(this->lock);
        return NULL;
    }
    this->expanded = true;
    release_lock(this->lock);
    this->expand(size / 0x1000 + 1);
    return this->malloc(size);
}

void *BuddyAlloc::calloc(size_t num, size_t size)
{
    void *ptr = this->malloc(num * size);
    if (!ptr) return NULL;

    memset(ptr, 0, num * size);
    return ptr;
}

void *BuddyAlloc::realloc(void *ptr, size_t size)
{
    if (!ptr) return this->malloc(size);

    BuddyBlock *block = (BuddyBlock*)((char*)ptr - sizeof(BuddyBlock));
    size_t oldsize = block->size;

    if (size == 0)
    {
        this->free(ptr);
        return NULL;
    }
    if (size < oldsize) oldsize = size;

    void *newptr = this->malloc(size);
    if (newptr == NULL) return ptr;

    memcpy(newptr, ptr, oldsize);
    this->free(ptr);
    return newptr;
}

void BuddyAlloc::free(void *ptr)
{
    if (ptr == NULL) return;

    ASSERT(this->head <= ptr, "Head is not smaller than pointer!");
    ASSERT(ptr < this->tail, "Pointer is not smaller than tail!");

    acquire_lock(this->lock);

    BuddyBlock *block = (BuddyBlock*)((char*)ptr - sizeof(BuddyBlock));
    block->free = true;

    if (this->debug) serial::info("Freed %zu bytes", block->size - sizeof(BuddyBlock));

    this->coalescence();

    release_lock(this->lock);
}

size_t BuddyAlloc::allocsize(void *ptr)
{
    if (!ptr) return 0;
    return ((BuddyBlock*)((char*)ptr - sizeof(BuddyBlock)))->size - sizeof(BuddyBlock);
}