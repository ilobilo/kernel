// Copyright (C) 2021  ilobilo

#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <kernel/kernel.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/lock.hpp>
#include <lib/cpu.hpp>
#include <lib/log.hpp>

namespace kernel::system::mm::vmm {

bool initialised = false;
Pagemap *kernel_pagemap = nullptr;
DEFINE_LOCK(vmm_lock)

PTable *get_next_lvl(PTable *curr_lvl, size_t entry)
{
    PTable *ret = nullptr;
    if (curr_lvl->entries[entry].getflag(Present))
    {
        ret = reinterpret_cast<PTable*>(static_cast<uint64_t>(curr_lvl->entries[entry].getAddr()) << 12);
    }
    else
    {
        ret = reinterpret_cast<PTable*>(pmm::alloc());
        curr_lvl->entries[entry].setAddr(reinterpret_cast<uint64_t>(ret) >> 12);
        curr_lvl->entries[entry].setflags(Present | ReadWrite | UserSuper, true);
    }
    return ret;
}

void Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    acquire_lock(vmm_lock);
    size_t pml4_entry = (vaddr & ((uint64_t)0x1FF << 39)) >> 39;
    size_t pml3_entry = (vaddr & ((uint64_t)0x1FF << 30)) >> 30;
    size_t pml2_entry = (vaddr & ((uint64_t)0x1FF << 21)) >> 21;
    size_t pml1_entry = (vaddr & ((uint64_t)0x1FF << 12)) >> 12;

    PTable *pml4 = this->PML4, *pml3, *pml2, *pml1;

    pml3 = get_next_lvl(pml4, pml4_entry);
    pml2 = get_next_lvl(pml3, pml3_entry);
    pml1 = get_next_lvl(pml2, pml2_entry);

    pml1->entries[pml1_entry].setAddr(paddr >> 12);
    pml1->entries[pml1_entry].setflags(flags, true);
    release_lock(vmm_lock);
}

void Pagemap::remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags)
{
    acquire_lock(vmm_lock);
    uint64_t paddr = 0;

    size_t pml4_entry = (vaddr_old & ((uint64_t)0x1FF << 39)) >> 39;
    size_t pml3_entry = (vaddr_old & ((uint64_t)0x1FF << 30)) >> 30;
    size_t pml2_entry = (vaddr_old & ((uint64_t)0x1FF << 21)) >> 21;
    size_t pml1_entry = (vaddr_old & ((uint64_t)0x1FF << 12)) >> 12;

    PTable *pml4 = this->PML4, *pml3, *pml2, *pml1;

    pml3 = get_next_lvl(pml4, pml4_entry);
    pml2 = get_next_lvl(pml3, pml3_entry);
    pml1 = get_next_lvl(pml2, pml2_entry);

    paddr = pml1->entries[pml1_entry].getAddr() << 12;
    pml1->entries[pml1_entry].value = 0;
    asm volatile ("invlpg (%0)" :: "r"(vaddr_old));

    release_lock(vmm_lock);
    this->mapMem(vaddr_new, paddr, flags);
}

void Pagemap::mapUserMem(uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    mapMem(vaddr, paddr, flags | UserSuper);
}

void Pagemap::unmapMem(uint64_t vaddr)
{
    acquire_lock(vmm_lock);
    size_t pml4_entry = (vaddr & ((uint64_t)0x1FF << 39)) >> 39;
    size_t pml3_entry = (vaddr & ((uint64_t)0x1FF << 30)) >> 30;
    size_t pml2_entry = (vaddr & ((uint64_t)0x1FF << 21)) >> 21;
    size_t pml1_entry = (vaddr & ((uint64_t)0x1FF << 12)) >> 12;

    PTable *pml4 = this->PML4, *pml3, *pml2, *pml1;

    pml3 = get_next_lvl(pml4, pml4_entry);
    pml2 = get_next_lvl(pml3, pml3_entry);
    pml1 = get_next_lvl(pml2, pml2_entry);

    pml1->entries[pml1_entry].value = 0;
    asm volatile ("invlpg (%0)" :: "r"(vaddr));
    release_lock(vmm_lock);
}

void Pagemap::setFlags(uint64_t vaddr, uint64_t flags)
{
    size_t pml4_entry = (vaddr & ((uint64_t)0x1FF << 39)) >> 39;
    size_t pml3_entry = (vaddr & ((uint64_t)0x1FF << 30)) >> 30;
    size_t pml2_entry = (vaddr & ((uint64_t)0x1FF << 21)) >> 21;
    size_t pml1_entry = (vaddr & ((uint64_t)0x1FF << 12)) >> 12;

    PTable *pml4 = this->PML4, *pml3, *pml2, *pml1;

    pml3 = get_next_lvl(pml4, pml4_entry);
    pml2 = get_next_lvl(pml3, pml3_entry);
    pml1 = get_next_lvl(pml2, pml2_entry);

    pml1->entries[pml1_entry].setflags(flags, true);
}

void Pagemap::remFlags(uint64_t vaddr, uint64_t flags)
{
    size_t pml4_entry = (vaddr & ((uint64_t)0x1FF << 39)) >> 39;
    size_t pml3_entry = (vaddr & ((uint64_t)0x1FF << 30)) >> 30;
    size_t pml2_entry = (vaddr & ((uint64_t)0x1FF << 21)) >> 21;
    size_t pml1_entry = (vaddr & ((uint64_t)0x1FF << 12)) >> 12;

    PTable *pml4 = this->PML4, *pml3, *pml2, *pml1;

    pml3 = get_next_lvl(pml4, pml4_entry);
    pml2 = get_next_lvl(pml3, pml3_entry);
    pml1 = get_next_lvl(pml2, pml2_entry);

    pml1->entries[pml1_entry].setflags(flags, false);
}

void PDEntry::setflag(PT_Flag flag, bool enabled)
{
    uint64_t bitSel = static_cast<uint64_t>(flag);
    value &= ~bitSel;
    if (enabled) value |= bitSel;
}

void PDEntry::setflags(uint64_t flags, bool enabled)
{
    uint64_t bitSel = flags;
    value &= ~bitSel;
    if (enabled) value |= bitSel;
}

bool PDEntry::getflag(PT_Flag flag)
{
    uint64_t bitSel = static_cast<uint64_t>(flag);
    return (value & (bitSel > 0)) ? true : false;
}

bool PDEntry::getflags(uint64_t flags)
{
    return (value & (flags > 0)) ? true : false;
}

uint64_t PDEntry::getAddr()
{
    return (value & 0x000FFFFFFFFFF000) >> 12;
}

void PDEntry::setAddr(uint64_t address)
{
    address &= 0x000000FFFFFFFFFF;
    value &= 0xFFF0000000000FFF;
    value |= (address << 12);
}

Pagemap *newPagemap()
{
    Pagemap *pagemap = new Pagemap;

    if (kernel_pagemap == nullptr)
    {
        pagemap->PML4 = reinterpret_cast<PTable*>(read_cr(3));
        return pagemap;
    }

    pagemap->PML4 = static_cast<PTable*>(pmm::alloc());

    PTable *pml4 = pagemap->PML4;
    PTable *kernel_pml4 = kernel_pagemap->PML4;
    for (size_t i = 0; i < 512; i++) pml4->entries[i] = kernel_pml4->entries[i];

    return pagemap;
}

Pagemap *clonePagemap(Pagemap *old)
{
    Pagemap *pagemap = new Pagemap;
    pagemap->PML4 = static_cast<PTable*>(pmm::alloc());

    PTable *pml4 = pagemap->PML4;
    PTable *old_pml4 = old->PML4;
    for (size_t i = 0; i < 512; i++) pml4->entries[i] = old_pml4->entries[i];

    return pagemap;
}

void switchPagemap(Pagemap *pmap)
{
    asm volatile ("mov %0, %%cr3" :: "r" (pmap->PML4) : "memory");
}

PTable *getPagemap()
{
    return reinterpret_cast<PTable*>(read_cr(3));
}

void init()
{
    log("Initialising VMM");

    if (initialised)
    {
        warn("VMM has already been initialised!\n");
        return;
    }

    kernel_pagemap = new Pagemap;
    kernel_pagemap->PML4 = static_cast<PTable*>(pmm::alloc());

    for (uint64_t i = 256; i < 512; i++)
    {
        get_next_lvl(kernel_pagemap->PML4, i);
    }

    for (uint64_t i = 0x1000; i < 0x100000000; i += 0x1000)
    {
        kernel_pagemap->mapMem(i, i);
        kernel_pagemap->mapMem(i + hhdm_tag->addr, i);
    }

    for (uint64_t i = 0; i < pmrs_tag->entries; i++)
    {
        uint64_t vaddr = pmrs_tag->pmrs[i].base;
        uint64_t paddr = kbaddr_tag->physical_base_address + (vaddr - kbaddr_tag->virtual_base_address);
        uint64_t flags = ((pmrs_tag->pmrs[i].permissions & STIVALE2_PMR_EXECUTABLE) ? 0 : NX) | ((pmrs_tag->pmrs[i].permissions & STIVALE2_PMR_WRITABLE) ? ReadWrite : 0) | Present;
        for (uint64_t t = 0; t < pmrs_tag->pmrs[i].length; i += 0x1000)
        {
            kernel_pagemap->mapMem(vaddr + t, paddr + t, flags);
        }
    }
    for (uint64_t i = 0; i < mmap_tag->entries; i++)
    {
        uint64_t base = ALIGN_DOWN(mmap_tag->memmap[i].base, 0x1000);
        uint64_t top = ALIGN_UP(mmap_tag->memmap[i].base + mmap_tag->memmap[i].length, 0x1000);
        if (top <= 0x100000000) continue;
        for (uint64_t t = base; t < top; t += 0x1000)
        {
            if (t < 0x100000000) continue;
            kernel_pagemap->mapMem(t, t);
            kernel_pagemap->mapMem(t + hhdm_tag->addr, t);
        }
    }
    switchPagemap(kernel_pagemap);

    serial::newline();
    initialised = true;
}
}