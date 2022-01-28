// Copyright (C) 2021-2022  ilobilo

#include <system/mm/pmm/pmm.hpp>
#include <lib/memory.hpp>
#include <lib/panic.hpp>
#include <lib/buddy.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>

using namespace kernel::system::mm;

BuddyBlock *BuddyAlloc::next(BuddyBlock *block)
{
    return reinterpret_cast<BuddyBlock*>(reinterpret_cast<uint8_t*>(block) + block->size);
}

BuddyBlock *BuddyAlloc::split(BuddyBlock *block, size_t size)
{
    if (block != nullptr && size != 0)
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
    return nullptr;
}

BuddyBlock *BuddyAlloc::find_best(size_t size)
{
    if (size == 0) return nullptr;

    BuddyBlock *best_block = nullptr;
    BuddyBlock *block = this->head;
    BuddyBlock *buddy = this->next(block);

    if (buddy == this->tail && block->free) return this->split(block, size);

    while (block < this->tail && buddy < this->tail)
    {
        if (block->free && buddy->free && block->size == buddy->size)
        {
            block->size <<= 1;
            if (size <= block->size && (best_block == nullptr || block->size <= best_block->size)) best_block = block;

            block = this->next(buddy);
            if (block < this->tail) buddy = this->next(block);
            continue;
        }

        if (block->free && size <= block->size && (best_block == nullptr || block->size <= best_block->size)) best_block = block;
        if (buddy->free && size <= buddy->size && (best_block == nullptr || buddy->size < best_block->size)) best_block = buddy;

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

    if (best_block != nullptr) return this->split(best_block, size);

    return nullptr;
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

void BuddyAlloc::init()
{
    this->lock.lock();

    size_t size = ALIGN_UP_2(0x1000, INIT_PAGES * 0x1000);
    ASSERT(POWER_OF_2(size), "Buddy: Size is not power of two!");

    this->data = pmm::alloc(size / 0x1000);
    ASSERT(this->data != nullptr, "Buddy: Could not allocate memory!");

    this->head = static_cast<BuddyBlock*>(this->data);
    this->head->size = size;
    this->head->free = true;

    this->tail = this->next(this->head);

    if (this->debug) log("Buddy: Initialised the heap. Current size: %zu bytes, %zu pages", size, size / 0x1000);
    this->lock.unlock();
}

void *BuddyAlloc::malloc(size_t size)
{
    if (size == 0) return nullptr;
    if (this->data == nullptr) this->init();

    this->lock.lock();

    size_t actual_size = this->required_size(size);

    BuddyBlock *found = this->find_best(actual_size);
    if (found == nullptr)
    {
        this->coalescence();
        found = this->find_best(actual_size);
    }

    if (found != nullptr)
    {
        if (this->debug) log("Buddy: Allocated %zu bytes", size);
        found->free = false;
        this->lock.unlock();
        return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(found) + sizeof(BuddyBlock));
    }

    error("Buddy: Could not allocate memory!");
    this->lock.unlock();
    return nullptr;
}

void *BuddyAlloc::calloc(size_t num, size_t size)
{
    void *ptr = this->malloc(num * size);
    if (!ptr) return nullptr;

    memset(ptr, 0, num * size);
    return ptr;
}

void *BuddyAlloc::realloc(void *ptr, size_t size)
{
    if (!ptr) return this->malloc(size);

    BuddyBlock *block = reinterpret_cast<BuddyBlock*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(BuddyBlock));
    size_t oldsize = block->size;

    if (size == 0)
    {
        this->free(ptr);
        return nullptr;
    }
    if (size < oldsize) oldsize = size;

    void *newptr = this->malloc(size);
    if (newptr == nullptr) return ptr;

    memcpy(newptr, ptr, oldsize);
    this->free(ptr);
    return newptr;
}

void BuddyAlloc::free(void *ptr)
{
    if (this->data == nullptr) return;
    if (ptr == nullptr) return;

    ASSERT(this->head <= ptr, "Buddy: Head is not smaller than pointer!");
    ASSERT(ptr < this->tail, "Buddy: Pointer is not smaller than tail!");

    this->lock.lock();

    BuddyBlock *block = reinterpret_cast<BuddyBlock*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(BuddyBlock));
    block->free = true;

    if (this->debug) log("Buddy: Freed %zu bytes", block->size - sizeof(BuddyBlock));

    this->coalescence();

    this->lock.unlock();
}

size_t BuddyAlloc::allocsize(void *ptr)
{
    if (this->data == nullptr) return 0;
    if (!ptr) return 0;
    return (reinterpret_cast<BuddyBlock*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(BuddyBlock)))->size - sizeof(BuddyBlock);
}