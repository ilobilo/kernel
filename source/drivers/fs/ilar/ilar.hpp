// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>
#include <cstddef>

namespace kernel::drivers::fs::ilar {

static constexpr size_t PATH_LENGTH = 128;
static constexpr char ILAR_SIGNATURE[] = "ILAR";

enum filetypes
{
    ILAR_REGULAR = 0,
    ILAR_DIRECTORY = 1,
    ILAR_SYMLINK = 2
};

struct [[gnu::packed]] fileheader
{
    char signature[5];
    char name[PATH_LENGTH];
    char link[PATH_LENGTH];
    uint64_t size;
    uint8_t type;
    uint32_t mode;
};

extern bool initialised;

void init(uint64_t module);
}