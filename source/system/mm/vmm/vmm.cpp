// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <kernel/main.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::mm::vmm {

bool initialised = false;
Pagemap *kernel_pagemap = NULL;

PTable *get_next_lvl(PTable *curr_lvl, size_t entry)
{
    PTable *ret;
    if (curr_lvl->entries[entry].getflag(Present))
    {
        ret = (PTable*)((uint64_t)curr_lvl->entries[entry].getAddr() << 12);
    }
    else
    {
        ret = (PTable*)pmm::requestPage();
        memset(ret, 0, 4096);
        curr_lvl->entries[entry].setAddr((uint64_t)ret >> 12);
        curr_lvl->entries[entry].setflags(Present | ReadWrite, true);
    }
    return ret;
}

void Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    size_t pml4_entry = (vaddr & ((uint64_t)0x1FF << 39)) >> 39;
    size_t pml3_entry = (vaddr & ((uint64_t)0x1FF << 30)) >> 30;
    size_t pml2_entry = (vaddr & ((uint64_t)0x1FF << 21)) >> 21;
    size_t pml1_entry = (vaddr & ((uint64_t)0x1FF << 12)) >> 12;

    PTable *pml4 = this->PML4, *pml3, *pml2, *pml1;

    pml3 = get_next_lvl(pml4, pml4_entry);
    pml2 = get_next_lvl(pml3, pml3_entry);
    if (flags & LargerPages)
    {
        pml2->entries[pml2_entry].setAddr(paddr >> 12);
        pml2->entries[pml2_entry].setflags(flags, true);
        return;
    }

    pml1 = get_next_lvl(pml2, pml2_entry);

    pml1->entries[pml1_entry].setAddr(paddr >> 12);
    pml1->entries[pml1_entry].setflags(flags, true);
}

void Pagemap::unmapMem(uint64_t vaddr)
{
    size_t pml4_entry = (vaddr & ((uint64_t)0x1FF << 39)) >> 39;
    size_t pml3_entry = (vaddr & ((uint64_t)0x1FF << 30)) >> 30;
    size_t pml2_entry = (vaddr & ((uint64_t)0x1FF << 21)) >> 21;
    size_t pml1_entry = (vaddr & ((uint64_t)0x1FF << 12)) >> 12;

    PTable *pml4 = this->PML4, *pml3, *pml2, *pml1;

    pml3 = get_next_lvl(pml4, pml4_entry);
    pml2 = get_next_lvl(pml3, pml3_entry);
    pml1 = get_next_lvl(pml2, pml2_entry);

    pml1->entries[pml1_entry].setAddr(0);
    pml1->entries[pml1_entry].setflags((Present | ReadWrite | UserSuper | WriteThrough | CacheDisable | Accessed | LargerPages | Custom1 | Custom2 | NX), false);
    asm volatile ("invlpg (%0)" :: "r"(vaddr));
}

void Pagemap::mapUserMem(uint64_t vaddr, uint64_t paddr, uint64_t flags)
{
    mapMem(vaddr, paddr, flags | UserSuper);
}

void PDEntry::setflag(PT_Flag flag, bool enabled)
{
    uint64_t bitSel = (uint64_t)flag;
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
    uint64_t bitSel = (uint64_t)flag;
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
    pagemap->PML4 = (PTable*)pmm::requestPage();

    PTable *pml4 = pagemap->PML4;
    PTable *kernel_pml4 = kernel_pagemap->PML4;
    for (size_t i = 0; i < 512; i++) pml4->entries[i] = kernel_pml4->entries[i];

    return pagemap;
}

Pagemap *clonePagemap(Pagemap *old)
{
    Pagemap *pagemap = new Pagemap;
    pagemap->PML4 = (PTable*)pmm::requestPage();

    PTable *pml4 = pagemap->PML4;
    PTable *old_pml4 = old->PML4;
    for (size_t i = 0; i < 512; i++) pml4->entries[i] = old_pml4->entries[i];

    return pagemap;
}

void switchPagemap(Pagemap *pmap)
{
    asm ("mov %0, %%cr3" : : "r" (pmap->PML4));
}

CRs getCRs()
{
    uint64_t cr0, cr2, cr3;
    asm volatile (
        "mov %%cr0, %%rax\n\t"
        "mov %%eax, %0\n\t"
        "mov %%cr2, %%rax\n\t"
        "mov %%eax, %1\n\t"
        "mov %%cr3, %%rax\n\t"
        "mov %%eax, %2\n\t"
    : "=m" (cr0), "=m" (cr2), "=m" (cr3)
    : /* no input */
    : "%rax"
    );
    return {cr0, cr2, cr3};
}

void init()
{
    serial::info("Initialising VMM");

    if (initialised)
    {
        serial::warn("VMM has already been initialised!\n");
        return;
    }

    kernel_pagemap->PML4 = (PTable*)getCRs().cr3;

    // kernel_pagemap->PML4 = (PTable*)pmm::requestPage();
    
    // for (uintptr_t p = 0; p < 0x100000000; p += 0x200000)
    // {
    //     kernel_pagemap->mapMem(p, p, Present | ReadWrite | LargerPages);
    //     kernel_pagemap->mapMem(p + hhdm_tag->addr, p, Present | ReadWrite | LargerPages);
    // }

    // for (size_t i = 0; i < pmrs_tag->entries; i++)
    // {
    //     uintptr_t vaddr = pmrs_tag->pmrs[i].base;
    //     uintptr_t paddr = kbaddr_tag->physical_base_address + (vaddr - kbaddr_tag->virtual_base_address);
    //     uintptr_t flags = ((pmrs_tag->pmrs[i].permissions & STIVALE2_PMR_EXECUTABLE) ? 0 : NX) | ((pmrs_tag->pmrs[i].permissions & STIVALE2_PMR_WRITABLE) ? ReadWrite : 0) | Present;
    //     for (uintptr_t p = 0; p < pmrs_tag->pmrs[i].length; p += 0x1000)
    //     {
    //         kernel_pagemap->mapMem(p + vaddr, p + paddr, flags);
    //     }
    // }

    // for (size_t i = 0; i < mmap_tag->entries; i++)
    // {
    //     uintptr_t base = ALIGN_DOWN(mmap_tag->memmap[i].base, 0x200000);
    //     uintptr_t top = ALIGN_UP(mmap_tag->memmap[i].base + mmap_tag->memmap[i].length, 0x200000);
    //     uintptr_t length = top - base;
    //     for (uintptr_t p = 0; p < length; p += 0x200000)
    //     {
    //         kernel_pagemap->mapMem(p + base, p + base, Present | ReadWrite | LargerPages);
    //         kernel_pagemap->mapMem(p + base + hhdm_tag->addr, p + base, Present | ReadWrite | LargerPages);
    //     }
    // }

    switchPagemap(kernel_pagemap);

    serial::newline();
    initialised = true;
}
}