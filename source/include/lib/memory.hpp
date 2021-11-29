// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

uint64_t getmemsize();

extern "C" void *memcpy(void *dest, void *src, size_t n);

extern "C" int memcmp(const void *s1, const void *s2, int len);

extern "C" void memset(void *str, char ch, size_t n);

extern "C" void memmove(void *dest, void *src, size_t n);