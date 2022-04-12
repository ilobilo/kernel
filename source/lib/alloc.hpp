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

extern "C" void *malloc(size_t size);
extern "C" void *calloc(size_t num, size_t size);
extern "C" void *realloc(void *ptr, size_t size);
extern "C" void free(void *ptr);
size_t allocsize(void *ptr);

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