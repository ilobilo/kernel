// Copyright (C) 2021  ilobilo

#pragma once

#include <system/mm/bitmap/bitmap.hpp>
#include <stdint.h>
#include <main.hpp>

namespace kernel::system::mm::pmm {

extern Bitmap PageBitmap;
extern bool initialised;

void freePage(void *address);
void lockPage(void *address);
void freePages(void *address, uint64_t pageCount);
void lockPages(void *address, uint64_t pageCount);

void *requestPage();
void *requestPages(uint64_t count);

uint64_t getFreeRam();
uint64_t getUsedRam();
uint64_t getReservedRam();

void Bitmap_init(size_t bitmapSize, uintptr_t bufferAddr);
void reservePage(void *address);
void unreservePage(void *address);
void reservePages(void *address, uint64_t pageCount);
void unreservePages(void *address, uint64_t pageCount);

void init();
}