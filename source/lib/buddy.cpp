// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <lib/memory.hpp>
#include <lib/panic.hpp>
#include <lib/buddy.hpp>
#include <lib/math.hpp>
#include <lib/lock.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

static BuddyBlock *head;
static BuddyBlock *tail;
static void *data = nullptr;
static volatile bool expanded = false;

bool buddy_debug = false;
size_t buddy_pages = 0;

DEFINE_LOCK(buddy_lock);

BuddyBlock *next(BuddyBlock *block)
{
    return (BuddyBlock*)((char*)block + block->size);
}

BuddyBlock *split(BuddyBlock *block, size_t size)
{
    if (block != NULL && size != 0)
    {
        while (size < block->size)
        {
            size_t sz = block->size >> 1;
            block->size = sz;
            block = next(block);
            block->size = sz;
            block->free = true;
        }
        if (size <= block->size) return block;
    }
    return NULL;
}

BuddyBlock *find_best(size_t size)
{
    if (size == 0) return NULL;

    BuddyBlock *best_block = NULL;
    BuddyBlock *block = head;
    BuddyBlock *buddy = next(block);

    if (buddy == tail && block->free) return split(block, size);

    while (block < tail && buddy < tail)
    {
        if (block->free && buddy->free && block->size == buddy->size)
        {
            block->size <<= 1;
            if (size <= block->size && (best_block == NULL || block->size <= best_block->size)) best_block = block;

            block = next(buddy);
            if (block < tail) buddy = next(block);
            continue;
        }

        if (block->free && size <= block->size && (best_block == NULL || block->size <= best_block->size)) best_block = block;
        if (buddy->free && size <= buddy->size && (best_block == NULL || buddy->size < best_block->size)) best_block = buddy;

        if (block->size <= buddy->size)
        {
            block = next(buddy);
            if (block < tail) buddy = next(block);
        }
        else
        {
            block = buddy;
            buddy = next(buddy);
        }
    }

    if (best_block != NULL) return split(best_block, size);

    return NULL;
}

size_t required_size(size_t size)
{
    size_t actual_size = sizeof(BuddyBlock);

    size += sizeof(BuddyBlock);
    size = ALIGN_UP(size, sizeof(BuddyBlock));

    while (size > actual_size) actual_size <<= 1;

    return actual_size;
}

void coalescence()
{
    while (true)
    {
        BuddyBlock *block = head;
        BuddyBlock *buddy = next(block);

        bool no_coalescence = true;
        while (block < tail && buddy < tail)
        {
            if (block->free && buddy->free && block->size == buddy->size)
            {
                block->size <<= 1;
                block = next(block);
                if (block < tail)
                {
                    buddy = next(block);
                    no_coalescence = false;
                }
            }
            else if (block->size < buddy->size)
            {
                block = buddy;
                buddy = next(buddy);
            }
            else
            {
                block = next(buddy);
                if (block < tail) buddy = next(block);
            }
        }

        if (no_coalescence) return;
    }
}

void buddy_init(size_t pagecount)
{
    buddy_expand(pagecount);
}

void buddy_expand(size_t pagecount)
{
    acquire_lock(buddy_lock);
    ASSERT(pagecount != 0, "Page count can not be zero!");
    size_t size = (pagecount + buddy_pages) * 0x1000;
    while (size % 0x1000 || !POWER_OF_2(size))
    {
        size++;
        size = to_power_of_2(size);
    }
    pagecount = size / 0x1000;
    ASSERT(POWER_OF_2(size), "Size is not power of two!");

    data = pmm::realloc(data, pagecount);
    ASSERT(data != NULL, "Could not allocate memory!");

    head = (BuddyBlock*)data;
    head->size = size;
    head->free = true;

    tail = next(head);
    buddy_pages = pagecount;

    if (buddy_debug) serial::info("Expanded the heap. Current size: %zu bytes, %zu pages", size, pagecount);
    release_lock(buddy_lock);
}

void buddy_setsize(size_t pagecount)
{
    ASSERT(pagecount != 0, "Page count can not be zero!");
    ASSERT(pagecount > buddy_pages, "Page count needs to be higher than current size!");
    pagecount = pagecount - buddy_pages;
    buddy_expand(pagecount);
}

void *malloc(size_t size)
{
    if (size == 0) return NULL;

    acquire_lock(buddy_lock);

    size_t actual_size = required_size(size);

    BuddyBlock *found = find_best(actual_size);
    if (found == NULL)
    {
        coalescence();
        found = find_best(actual_size);
    }

    if (found != NULL)
    {
        if (buddy_debug) serial::info("Malloc: Allocated %zu bytes");
        found->free = false;
        expanded = false;
        release_lock(buddy_lock);
        return (void*)((char*)found + sizeof(BuddyBlock));
    }

    if (expanded)
    {
        if (buddy_debug) serial::err("Malloc: Could not expand the heap!");
        expanded = false;
        release_lock(buddy_lock);
        return NULL;
    }
    expanded = true;
    release_lock(buddy_lock);
    buddy_expand(size / 0x1000 + 1);
    return malloc(size);
}

void *calloc(size_t num, size_t size)
{
    void *ptr = malloc(num * size);
    if (!ptr) return NULL;

    memset(ptr, 0, num * size);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr) return malloc(size);

    BuddyBlock *block = (BuddyBlock*)((char*)ptr - sizeof(BuddyBlock));
    size_t oldsize = block->size;

    if (size == 0)
    {
        free(ptr);
        return NULL;
    }
    if (size < oldsize) oldsize = size;

    void *newptr = malloc(size);
    if (newptr == NULL) return ptr;

    memcpy(newptr, ptr, oldsize);
    free(ptr);
    return newptr;
}

void free(void *ptr)
{
    if (ptr == NULL) return;

    ASSERT(head <= ptr, "Head is not smaller than pointer!");
    ASSERT(ptr < tail, "Pointer is not smaller than tail!");

    acquire_lock(buddy_lock);

    BuddyBlock *block = (BuddyBlock*)((char*)ptr - sizeof(BuddyBlock));
    block->free = true;

    if (buddy_debug) serial::info("Freed %zu bytes", block->size - sizeof(BuddyBlock));

    coalescence();

    release_lock(buddy_lock);
}

size_t allocsize(void *ptr)
{
    if (!ptr) return 0;
    return ((BuddyBlock*)((char*)ptr - sizeof(BuddyBlock)))->size - sizeof(BuddyBlock);
}

void *operator new(size_t size)
{
    return malloc(size);
}

void *operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void *ptr)
{
    free(ptr);
}

void operator delete[](void *ptr)
{
    free(ptr);
}