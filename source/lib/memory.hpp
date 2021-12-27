// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

uint64_t getmemsize();

extern "C" void *memcpy(void *dest, const void *src, size_t len);
extern "C" int memcmp(const void *ptr1, const void *ptr2, size_t len);
extern "C" void *memset(void *dest, int ch, size_t n);
extern "C" void *memmove(void *dest, const void *src, size_t len);