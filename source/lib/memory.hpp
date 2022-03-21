// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>
#include <cstddef>

uint64_t getmemsize();

bool arraycmp(uint8_t *a1, uint8_t *a2, size_t length);

extern "C" void *memcpy(void *dest, const void *src, size_t len);
extern "C" int memcmp(const void *ptr1, const void *ptr2, size_t len);
extern "C" void *memset(void *dest, int ch, size_t n);
extern "C" void *memmove(void *dest, const void *src, size_t len);