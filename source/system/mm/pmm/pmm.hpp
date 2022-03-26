// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/bitmap.hpp>
#include <limine.h>
#include <cstdint>

namespace kernel::system::mm::pmm {

extern Bitmap bitmap;
extern bool initialised;

extern limine_memmap_entry **memmaps;
extern uint64_t memmap_count;

void *alloc(size_t count = 1);
void *realloc(void *ptr, size_t oldcount = 1, size_t newcount = 1);
void free(void *ptr, size_t count = 1);

size_t freemem();
size_t usedmem();

void init();
}