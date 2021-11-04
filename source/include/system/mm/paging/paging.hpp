// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::system::mm::paging {

enum PT_Flag
{
    Present = 0,
    ReadWrite = 1,
    UserSuper = 2,
    WriteThrough = 3,
    CacheDisable = 4,
    Accessed = 5,
    LargerPages = 7,
    Custom0 = 9,
    Custom1 = 10,
    Custom2 = 11,
    NX = 63
};

struct PDEntry
{
    uint64_t value;
    void setflag(PT_Flag flag, bool enabled);
    bool getflag(PT_Flag flag);
    void setAddr(uint64_t address);
    uint64_t getAddr();
};

struct PTable
{
    PDEntry entries[512];
} __attribute__((aligned(0x1000)));

}