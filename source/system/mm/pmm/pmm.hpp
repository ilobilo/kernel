// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/bitmap.hpp>
#include <kernel/main.hpp>
#include <stdint.h>

namespace kernel::system::mm::pmm {

extern Bitmap bitmap;
extern bool initialised;

void *alloc(size_t count = 1);
void *realloc(void *ptr, size_t oldcount = 1, size_t newcount = 1);
void free(void *ptr, size_t count = 1);

size_t freemem();
size_t usedmem();

void init();
}