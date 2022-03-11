// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stddef.h>
#include <stdint.h>

struct Bitmap
{
    uint8_t *buffer = nullptr;
    bool operator[](uint64_t index);
    bool Set(uint64_t index, bool value);
    bool Get(uint64_t index);
};