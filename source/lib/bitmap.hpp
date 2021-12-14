// Copyright (C) 2021  ilobilo

#pragma once

#include <stddef.h>
#include <stdint.h>

class Bitmap
{
    public:
    uint8_t *buffer;
    bool operator[](uint64_t index);
    bool Set(uint64_t index, bool value);
    bool Get(uint64_t index);
};