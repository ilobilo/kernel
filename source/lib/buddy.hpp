// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

struct BuddyBlock
{
    size_t size;
    bool free;
};

extern bool buddy_debug;
extern size_t buddy_pages;

void buddy_init(size_t pagecount = 16);

void buddy_expand(size_t pagecount);
void buddy_setsize(size_t pagecount);

void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

size_t allocsize(void *ptr);

void *operator new(size_t size);
void *operator new[](size_t size);

void operator delete(void *ptr);
void operator delete[](void *ptr);