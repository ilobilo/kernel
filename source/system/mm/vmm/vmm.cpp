// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/framebuffer/framebuffer.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <kernel/kernel.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/cpu.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::mm::vmm {

bool initialised = false;
bool lvl5 = LVL5_PAGING;
Pagemap *kernel_pagemap = nullptr;

static PTable *get_next_lvl(PTable *curr_lvl, size_t entry, bool allocate = true)
{
    PTable *ret = nullptr;
    if (curr_lvl->entries[entry].getflag(Present))
    {
        ret = reinterpret_cast<PTable*>(static_cast<uint64_t>(curr_lvl->entries[entry].getAddr()) << 12);
    }
    else if (allocate == true)
    {
        ret = reinterpret_cast<PTable*>(pmm::alloc());
        curr_lvl->entries[entry].setAddr(reinterpret_cast<uint64_t>(ret) >> 12);
        curr_lvl->entries[entry].setflags(Present | ReadWrite | UserSuper, true);
    }
    return ret;
}

PDEntry *Pagemap::virt2pte(uint64_t vaddr, bool allocate)
{
    size_t pml5_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 48)) >> 48;
    size_t pml4_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 39)) >> 39;
    size_t pml3_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 30)) >> 30;
    size_t pml2_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 21)) >> 21;
    size_t pml1_entry = (vaddr & (static_cast<uint64_t>(0x1FF) << 12)) >> 12;

    PTable *pml5, *pml4, *pml3, *pml2, *pml1;

    if (lvl5)
    {
        pml5 = this->TOPLVL;
        pml4 = get_next_lvl(pml5, pml5_entry, allocate);
    }
    else
    {
        pml5 = nullptr;
        pml4 = this->TOPLVL;
    }
    pml3 = get_next_lvl(pml4, pml4_entry, allocate);
    pml2 = get_next_lvl(pml3, pml3_entry, allocate);
    pml1 = get_next_lvl(pml2, pml2_entry, allocate);
    return &pml1->entries[pml1_entry];
}

void Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    this->lock.lock();
    PDEntry *pml1_entry = this->virt2pte(vaddr);
    if (pml1_entry == nullptr)
    {
        error("VMM: Could not get page map entry!");
        this->lock.unlock();
        return;
    }

    pml1_entry->setAddr(paddr >> 12);
    pml1_entry->setflags(flags, true);
    this->lock.unlock();
}

void Pagemap::mapMemRange(uint64_t vaddr, uint64_t paddr, uint64_t pagecount, uint64_t flags)
{
    for (size_t i = 0; i < pagecount; i += 0x1000)
    {
        this->mapMem(vaddr + i, paddr + i, flags);
    }
}

void Pagemap::remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags)
{
    this->lock.lock();
    PDEntry *pml1_entry = this->virt2pte(vaddr_old, false);
    if (pml1_entry == nullptr)
    {
        error("VMM: Could not get page map entry!");
        this->lock.unlock();
        return;
    }

    uint64_t paddr = pml1_entry->getAddr() << 12;
    pml1_entry->value = 0;
    invlpg(vaddr_old);
    this->lock.unlock();

    this->mapMem(vaddr_new, paddr, flags);
}

void Pagemap::unmapMem(uint64_t vaddr)
{
    this->lock.lock();
    PDEntry *pml1_entry = this->virt2pte(vaddr, false);
    if (pml1_entry == nullptr)
    {
        error("VMM: Could not get page map entry!");
        this->lock.unlock();
        return;
    }

    pml1_entry->value = 0;
    invlpg(vaddr);
    this->lock.unlock();
}

void Pagemap::unmapMemRange(uint64_t vaddr, uint64_t pagecount)
{
    for (size_t i = 0; i < pagecount; i += 0x1000)
    {
        this->unmapMem(vaddr);
    }
}

void Pagemap::setflags(uint64_t vaddr, uint64_t flags, bool enabled)
{
    this->lock.lock();
    PDEntry *pml1_entry = this->virt2pte(vaddr, false);
    if (pml1_entry == nullptr)
    {
        error("VMM: Could not get page map entry!");
        this->lock.unlock();
        return;
    }
    pml1_entry->setflags(flags, enabled);
    this->lock.unlock();
}

bool Pagemap::getflags(uint64_t vaddr, uint64_t flags)
{
    this->lock.lock();
    PDEntry *pml1_entry = this->virt2pte(vaddr, false);
    if (pml1_entry == nullptr)
    {
        error("VMM: Could not get page map entry!");
        this->lock.unlock();
        return false;
    };
    this->lock.unlock();
    return pml1_entry->getflags(flags);
}

Pagemap *newPagemap()
{
    Pagemap *pagemap = new Pagemap;

    pagemap->TOPLVL = static_cast<PTable*>(pmm::alloc());

    if (kernel_pagemap == nullptr)
    {
        for (size_t i = 256; i < 512; i++) get_next_lvl(pagemap->TOPLVL, i);

        for (size_t i = 0; i < pmrs_tag->entries; i++)
        {
            stivale2_pmr &pmr = pmrs_tag->pmrs[i];

            uint64_t vaddr = pmr.base;
            uint64_t paddr = kbad_tag->physical_base_address + (vaddr - kbad_tag->virtual_base_address);
            uint64_t length = pmr.length;

            for (uint64_t t = 0; t < length; t += 0x1000)
            {
                pagemap->mapMem(vaddr + t, paddr + t);
            }
        }

        for (uint64_t i = 0x1000; i < 0x100000000; i += 0x1000)
        {
            pagemap->mapMem(i, i);
            pagemap->mapMem(i + hhdm_tag->addr, i);
        }

        for (size_t i = 0; i < mmap_tag->entries; i++)
        {
            stivale2_mmap_entry &mmap = mmap_tag->memmap[i];

            uint64_t base = ALIGN_DOWN(mmap.base, 0x1000);
            uint64_t top = ALIGN_UP(mmap.base + mmap.length, 0x1000);
            if (top < 0x100000000) continue;

            for (uint64_t t = base; t < top; t += 0x1000)
            {
                if (t < 0x100000000) continue;

                pagemap->mapMem(t, t);
                pagemap->mapMem(t + hhdm_tag->addr, t);
            }
        }

        pagemap->mapMem(framebuffer::frm_addr, framebuffer::frm_addr - hhdm_tag->addr, Present | ReadWrite | CacheDisable);

        return pagemap;
    }

    PTable *toplvl = reinterpret_cast<PTable*>(reinterpret_cast<uint64_t>(pagemap->TOPLVL) + hhdm_tag->addr);
    PTable *kerenltoplvl = reinterpret_cast<PTable*>(reinterpret_cast<uint64_t>(kernel_pagemap->TOPLVL) + hhdm_tag->addr);
    for (size_t i = 0; i < 512; i++) toplvl[i] = kerenltoplvl[i];

    return pagemap;
}

Pagemap *clonePagemap(Pagemap *old)
{
    Pagemap *pagemap = new Pagemap;
    pagemap->TOPLVL = static_cast<PTable*>(pmm::alloc());

    PTable *pml4 = pagemap->TOPLVL;
    PTable *old_pml4 = old->TOPLVL;
    for (size_t i = 0; i < 512; i++) pml4->entries[i] = old_pml4->entries[i];

    return pagemap;
}

void switchPagemap(Pagemap *pmap)
{
    write_cr(3, reinterpret_cast<uint64_t>(pmap->TOPLVL));
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