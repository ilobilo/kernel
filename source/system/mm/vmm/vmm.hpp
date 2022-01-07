// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <stdint.h>
#include <stddef.h>

namespace kernel::system::mm::vmm {

enum PT_Flag
{
    Present = ((uint64_t)1 << 0),
    ReadWrite = ((uint64_t)1 << 1),
    UserSuper = ((uint64_t)1 << 2),
    WriteThrough = ((uint64_t)1 << 3),
    CacheDisable = ((uint64_t)1 << 4),
    Accessed = ((uint64_t)1 << 5),
    LargerPages = ((uint64_t)1 << 7),
    Custom0 = ((uint64_t)1 << 9),
    Custom1 = ((uint64_t)1 << 10),
    Custom2 = ((uint64_t)1 << 11),
    NX = ((uint64_t)1 << 63)
};

struct PDEntry
{
    uint64_t value;

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
    volatile lock_t lock;
    PTable *PML;

    PDEntry &virt2pte(uint64_t vaddr);

    void mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | ReadWrite));
    void remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags = (Present | ReadWrite));
    void mapUserMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | ReadWrite));
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