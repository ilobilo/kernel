// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <stdint.h>
#include <stddef.h>

namespace kernel::system::mm::vmm {

enum PT_Flag
{
    Present = (1 << 0),
    ReadWrite = (1 << 1),
    UserSuper = (1 << 2),
    WriteThrough = (1 << 3),
    CacheDisable = (1 << 4),
    Accessed = (1 << 5),
    LargerPages = (1 << 7),
    Custom0 = (1 << 9),
    Custom1 = (1 << 10),
    Custom2 = (1 << 11),
    NX = (1UL << 63)
};

struct PDEntry
{
    uint64_t value = 0;

    void setflag(PT_Flag flag, bool enabled);
    void setflags(uint64_t flags, bool enabled);

    bool getflag(PT_Flag flag);
    bool getflags(uint64_t flags);

    void setAddr(uint64_t address);
    uint64_t getAddr();
};

struct [[gnu::aligned(0x1000)]] PTable
{
    PDEntry entries[512];
};

struct Pagemap
{
    volatile lock_t lock = false;
    PTable *TOPLVL = nullptr;

    PDEntry &virt2pte(uint64_t vaddr);

    void mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | ReadWrite));
    void remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags = (Present | ReadWrite));
    void mapUserMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | ReadWrite));
    void mapHHMem(uint64_t paddr, uint64_t flags = (Present | ReadWrite));
    void unmapMem(uint64_t vaddr);

    void setFlags(uint64_t vaddr, uint64_t flags);
    void remFlags(uint64_t vaddr, uint64_t flags);
};

extern bool initialised;
extern bool lvl5;
extern Pagemap *kernel_pagemap;

Pagemap *newPagemap();
Pagemap *clonePagemap(Pagemap *old);
void switchPagemap(Pagemap *pmap);
PTable *getPagemap();

void init();
}