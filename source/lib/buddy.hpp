// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <stdint.h>
#include <stddef.h>

static constexpr uint64_t INIT_PAGES = 1024;

struct BuddyBlock
{
    size_t size;
    bool free;
};

class BuddyAlloc
{
    private:
    BuddyBlock *head = nullptr;
    BuddyBlock *tail = nullptr;
    void *data = nullptr;
    lock_t lock;

    BuddyBlock *next(BuddyBlock *block);
    BuddyBlock *split(BuddyBlock *block, size_t size);

    BuddyBlock *find_best(size_t size);
    size_t required_size(size_t size);
    void coalescence();

    public:
    bool debug = false;

    void init();

    void *malloc(size_t size);
    void *calloc(size_t num, size_t size);
    void *realloc(void *ptr, size_t size);
    void free(void *ptr);

    size_t allocsize(void *ptr);
};