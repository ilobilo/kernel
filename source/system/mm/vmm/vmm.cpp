// Copyright (C) 2021  ilobilo

#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <kernel/kernel.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/cpu.hpp>
#include <lib/log.hpp>

namespace kernel::system::mm::vmm {

bool initialised = false;
Pagemap *kernel_pagemap = nullptr;

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

PDEntry &Pagemap::virt2pte(uint64_t vaddr)
{
    size_t pml4_entry = (vaddr & ((uint64_t)0x1FF << 39)) >> 39;
    size_t pml3_entry = (vaddr & ((uint64_t)0x1FF << 30)) >> 30;
    size_t pml2_entry = (vaddr & ((uint64_t)0x1FF << 21)) >> 21;
    size_t pml1_entry = (vaddr & ((uint64_t)0x1FF << 12)) >> 12;

    PTable *pml4 = this->PML4, *pml3, *pml2, *pml1;

    pml3 = get_next_lvl(pml4, pml4_entry);
    pml2 = get_next_lvl(pml3, pml3_entry);
    pml1 = get_next_lvl(pml2, pml2_entry);
    return pml1->entries[pml1_entry];
}

void Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    acquire_lock(this->lock);
    PDEntry &pml1_entry = this->virt2pte(vaddr);

    pml1_entry.value = 0;
    pml1_entry.setAddr(paddr >> 12);
    pml1_entry.setflags(flags, true);
    release_lock(this->lock);
}

void Pagemap::remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags)
{
    acquire_lock(this->lock);
    PDEntry &pml1_entry = this->virt2pte(vaddr_old);

    uint64_t paddr = pml1_entry.getAddr() << 12;
    pml1_entry.value = 0;
    asm volatile ("invlpg (%0)" :: "r"(vaddr_old));
    release_lock(this->lock);
    this->mapMem(vaddr_new, paddr, flags);
}

void Pagemap::mapUserMem(uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    this->mapMem(vaddr, paddr, flags | UserSuper);
}

void Pagemap::unmapMem(uint64_t vaddr)
{
    acquire_lock(this->lock);
    this->virt2pte(vaddr).value = 0;
    asm volatile ("invlpg (%0)" :: "r"(vaddr));
    release_lock(this->lock);
}

void Pagemap::setFlags(uint64_t vaddr, uint64_t flags)
{
    acquire_lock(this->lock);
    this->virt2pte(vaddr).setflags(flags, true);
    release_lock(this->lock);
}

void Pagemap::remFlags(uint64_t vaddr, uint64_t flags)
{
    acquire_lock(this->lock);
    this->virt2pte(vaddr).setflags(flags, false);
    release_lock(this->lock);
}

void PDEntry::setflag(PT_Flag flag, bool enabled)
{
    uint64_t bitSel = static_cast<uint64_t>(flag);
    this->value &= ~bitSel;
    if (enabled)this-> value |= bitSel;
}

void PDEntry::setflags(uint64_t flags, bool enabled)
{
    uint64_t bitSel = flags;
    this->value &= ~bitSel;
    if (enabled) this->value |= bitSel;
}

bool PDEntry::getflag(PT_Flag flag)
{
    uint64_t bitSel = static_cast<uint64_t>(flag);
    return (this->value & (bitSel > 0)) ? true : false;
}

bool PDEntry::getflags(uint64_t flags)
{
    return (this->value & (flags > 0)) ? true : false;
}

uint64_t PDEntry::getAddr()
{
    return (this->value & 0x000FFFFFFFFFF000) >> 12;
}

void PDEntry::setAddr(uint64_t address)
{
    address &= 0x000000FFFFFFFFFF;
    this->value &= 0xFFF0000000000FFF;
    this->value |= (address << 12);
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
    write_cr(3, reinterpret_cast<uint64_t>(pmap->PML4));
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

    kernel_pagemap = newPagemap();
    switchPagemap(kernel_pagemap);

    serial::newline();
    initialised = true;
}
}