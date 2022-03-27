// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/vfs/vfs.hpp>
#include <lib/lock.hpp>
#include <cstdint>
#include <cstddef>

namespace kernel::system::mm::vmm {

static constexpr uint64_t large_page_size = 0x200000;
static constexpr uint64_t page_size = 0x1000;

enum PT_Flag
{
    Present = (1 << 0),
    ReadWrite = (1 << 1),
    UserSuper = (1 << 2),
    WriteThrough = (1 << 3),
    CacheDisable = (1 << 4),
    Accessed = (1 << 5),
    LargerPages = (1 << 7),
    PAT = (1 << 7),
    Custom0 = (1 << 9),
    Custom1 = (1 << 10),
    Custom2 = (1 << 11),
    NX = (1UL << 63)
};

enum mmap_flags
{
    ProtNone = 0x00,
    ProtRead = 0x01,
    ProtWrite = 0x02,
    ProtExec = 0x04,

    MapPrivate = 0x01,
    MapShared = 0x02,
    MapFixed = 0x04,
    MapAnon = 0x08
};

struct PDEntry
{
    uint64_t value = 0;

    void setflag(PT_Flag flag, bool enabled)
    {
        uint64_t bitSel = static_cast<uint64_t>(flag);
        this->value &= ~bitSel;
        if (enabled) this->value |= bitSel;
    }
    void setflags(uint64_t flags, bool enabled)
    {
        uint64_t bitSel = flags;
        this->value &= ~bitSel;
        if (enabled) this->value |= bitSel;
    }

    bool getflag(PT_Flag flag)
    {
        uint64_t bitSel = static_cast<uint64_t>(flag);
        return (this->value & bitSel) ? true : false;
    }
    bool getflags(uint64_t flags)
    {
        return (this->value & flags) ? true : false;
    }

    uint64_t getAddr()
    {
        return (this->value & 0x000FFFFFFFFFF000) >> 12;
    }
    void setAddr(uint64_t address)
    {
        address &= 0x000000FFFFFFFFFF;
        this->value &= 0xFFF0000000000FFF;
        this->value |= (address << 12);
    }
};

struct [[gnu::aligned(0x1000)]] PTable
{
    PDEntry entries[512];
};

struct Pagemap;
struct mmap_range_global;
struct mmap_range_local
{
    Pagemap *pagemap;
    mmap_range_global *global;
    uint64_t base;
    uint64_t length;
    int64_t offset;
    int prot;
    int flags;
};

struct Pagemap
{
    lock_t lock;
    PTable *TOPLVL = nullptr;
    vector<mmap_range_local*> ranges;

    PDEntry *virt2pte(uint64_t vaddr, bool allocate = true, bool hugepages = false);
    uint64_t virt2phys(uint64_t vaddr, bool hugepages = false)
    {
        PDEntry *pml_entry = this->virt2pte(vaddr, false, hugepages);
        if (pml_entry == nullptr || !pml_entry->getflag(Present)) return 0;

        return pml_entry->getAddr() << 12;
    }

    void mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags = (Present | ReadWrite), bool hugepages = false);
    void mapMemRange(uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t flags = (Present | ReadWrite), bool hugepages = false);

    bool remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags = (Present | ReadWrite));

    bool unmapMem(uint64_t vaddr, bool hugepages = false);
    void unmapMemRange(uint64_t vaddr, uint64_t pagecount, bool hugepages = false);

    auto addr2range(uint64_t addr)
    {
        struct ret { mmap_range_local *local; uint64_t mem_page; uint64_t file_page; };

        for (auto range : this->ranges)
        {
            if (addr >= range->base && addr < range->base + range->length)
            {
                uint64_t mem_page = addr / page_size;
                uint64_t file_page = range->offset / page_size + (mem_page - range->base / page_size);
                return ret { range, mem_page, file_page };
            }
        }

        return ret { nullptr, 0, 0 };
    }

    void mapRange(uint64_t vaddr, uint64_t paddr, uint64_t length, int prot, int flags);
    void *mmap(void *addr, uint64_t length, int prot, int flags, vfs::resource_t *res, int64_t offset);
    bool munmap(void *addr, uint64_t length);

    Pagemap *fork();
    void deleteThis();
    void switchTo();
};

struct mmap_range_global
{
    Pagemap shadow_pagemap;
    vector<mmap_range_local*> locals;
    vfs::resource_t *res;
    uint64_t base;
    uint64_t length;
    int64_t offset;

    void map_in_range(uint64_t vaddr, uint64_t paddr, int prot);
};

extern bool initialised;
extern bool lvl5;
extern Pagemap *kernel_pagemap;

Pagemap *newPagemap();
PTable *getPagemap();

void init();
}