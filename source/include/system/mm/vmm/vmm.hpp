// Copyright (C) 2021  ilobilo

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

struct PTable
{
    PDEntry entries[512];
} __attribute__((aligned(0x1000)));

class Pagemap
{
    public:
    PTable *PML4;

    void mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | ReadWrite));
    void mapUserMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | ReadWrite));
    void unmapMem(uint64_t vaddr);
};

struct CRs
{
    uint64_t cr0;
    uint64_t cr2;
    uint64_t cr3;
};

extern bool initialised;
extern Pagemap *kernel_pagemap;

Pagemap *newPagemap();
void switchPagemap(Pagemap *pmap);

CRs getCRs();

void init();
}