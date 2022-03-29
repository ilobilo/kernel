// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
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

void mmap_range_global::map_in_range(uint64_t vaddr, uint64_t paddr, int prot)
{
    uint64_t flags = Present | UserSuper;
    if (prot & ProtWrite) flags |= ReadWrite;
    this->shadow_pagemap.mapMem(vaddr, paddr, flags);

    for (auto local : this->locals)
    {
        if (vaddr < local->base || vaddr >= local->base + local->length) continue;
        local->pagemap->mapMem(vaddr, paddr, flags);
    }
}

void Pagemap::mapRange(uint64_t vaddr, uint64_t paddr, uint64_t length, int prot, int flags)
{
    flags |= MapAnon;

    length = ALIGN_UP(length + (vaddr - ALIGN_DOWN(vaddr, page_size)), page_size);
    vaddr = ALIGN_DOWN(vaddr, page_size);

    auto local = new mmap_range_local
    {
        .pagemap = this,
        .base = vaddr,
        .length = length,
        .prot = prot,
        .flags = flags
    };

    auto global = new mmap_range_global
    {
        .base = vaddr,
        .length = length
    };

    local->global = global;
    global->locals.push_back(local);
    global->shadow_pagemap.TOPLVL = static_cast<PTable*>(pmm::alloc());

    this->lock.lock();
    this->ranges.push_back(local);
    this->lock.unlock();

    for (size_t i = 0; i < length; i += page_size)
    {
        global->map_in_range(vaddr + i, paddr + i, prot);
    }
}

void *Pagemap::mmap(void *addr, uint64_t length, int prot, int flags, vfs::resource_t *res, int64_t offset)
{
    if (length == 0)
    {
        errno_set(EINVAL);
        return nullptr;
    }
    if ((flags & MapAnon) == 0 && res->can_mmap == false)
    {
        errno_set(ENODEV);
        return nullptr;
    }

    length = ALIGN_UP(length, page_size);

    uint64_t base = 0;
    if (flags & MapFixed)
    {
        base = reinterpret_cast<uint64_t>(addr);
        this->munmap(addr, length);
    }
    else
    {
        base = this_proc()->mmap_anon_base;
        this_proc()->mmap_anon_base += length + page_size;
    }

    auto local = new mmap_range_local
    {
        .pagemap = this,
        .base = base,
        .length = length,
        .offset = offset,
        .prot = prot,
        .flags = flags
    };

    auto global = new mmap_range_global
    {
        .res = res,
        .base = base,
        .length = length,
        .offset = offset
    };

    local->global = global;
    global->locals.push_back(local);
    global->shadow_pagemap.TOPLVL = static_cast<PTable*>(pmm::alloc());

    this->lock.lock();
    this->ranges.push_back(local);
    this->lock.unlock();

    if (res != nullptr) res->refcount++;
    return reinterpret_cast<void*>(base);
}

bool Pagemap::munmap(void *addr, uint64_t length)
{
    if (length == 0)
    {
        errno_set(EINVAL);
        return false;
    }
    length = ALIGN_UP(length, page_size);

    uint64_t address = reinterpret_cast<uint64_t>(addr);
    for (uint64_t i = address; i < address + length; i += page_size)
    {
        auto local = this->addr2range(i).local;
        if (local == nullptr) continue;

        auto global = local->global;
        uint64_t snip_begin = i;
        while (true)
        {
            i += page_size;
            if (i >= local->base + local->length || i >= address + length) break;
        }
        uint64_t snip_end = i;
        uint64_t snip_size = snip_end - snip_begin;

        if (snip_begin > local->base && snip_end < local->base + local->length)
        {
            auto range = new mmap_range_local
            {
                .pagemap = local->pagemap,
                .global = local->global,
                .base = snip_end,
                .length = (local->base + local->length) - snip_end,
                .offset = local->offset + static_cast<int64_t>(snip_end - local->base),
                .prot = local->prot,
                .flags = local->flags,
            };
            this->ranges.push_back(range);
            local->length -= range->length;
        }

        for (size_t p = 0; p < snip_end; p += page_size) this->unmapMem(p);
        if (snip_size == local->length) this->ranges.remove(local);
        if (snip_size == local->length && global->locals.size() == 1)
        {
            if (local->flags & MapAnon)
            {
                for (size_t p = global->base; p < global->base + global->length; p += page_size)
                {
                    uint64_t paddr = global->shadow_pagemap.virt2phys(p);
                    if (!global->shadow_pagemap.unmapMem(p))
                    {
                        errno_set(EINVAL);
                        return false;
                    }
                    pmm::free(reinterpret_cast<void*>(paddr));
                }
            }
            // else global->res->munmap(i);
            delete local;
        }
        else
        {
            if (snip_begin == local->base)
            {
                local->offset += snip_size;
                local->base = snip_end;
            }
            local->length -= snip_size;
        }
    }

    return true;
}

Pagemap *Pagemap::fork()
{
    lockit(this->lock);
    Pagemap *newpagemap = newPagemap();

    for (auto local : this->ranges)
    {
        auto global = local->global;
        auto newlocal = new mmap_range_local;
        *newlocal = *local;

        if (global->res) global->res->refcount++;
        if (local->flags & MapShared)
        {
            newlocal->global = global;
            global->locals.push_back(newlocal);
            for (size_t i = local->base; i < local->base + local->length; i += page_size)
            {
                auto oldpml = this->virt2pte(i, false);
                if (oldpml == nullptr) continue;

                auto newpml = newpagemap->virt2pte(i, true);
                if (newpml == nullptr) return nullptr;
                newpml->value = oldpml->value;
            }
        }
        else
        {
            auto newglobal = new mmap_range_global;

            newglobal->res = global->res;
            newglobal->base = global->base;
            newglobal->length = global->length;
            newglobal->offset = global->offset;
            newglobal->locals.push_back(newlocal);
            newglobal->shadow_pagemap.TOPLVL = static_cast<PTable*>(pmm::alloc());

            if (local->flags & MapAnon)
            {
                for (size_t i = local->base; i < local->base + local->length; i += page_size)
                {
                    auto oldpml = this->virt2pte(i, false);
                    if (oldpml == nullptr || !oldpml->getflag(Present)) continue;

                    auto newpml = newpagemap->virt2pte(i, true);
                    if (newpml == nullptr) return nullptr;

                    auto newshadowpml = newglobal->shadow_pagemap.virt2pte(i, true);
                    if (newpml == nullptr) return nullptr;

                    void *page = pmm::alloc();
                    memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(page) + hhdm_offset), reinterpret_cast<void*>((oldpml->getAddr() << 12) + hhdm_offset), page_size);
                    newpml->value = (oldpml->value & 0xFFFUL) | reinterpret_cast<uint64_t>(page);
                    newshadowpml->value = newpml->value;
                }
            }
            else panic("Non-anonymous fork!");
        }
        newpagemap->ranges.push_back(newlocal);
    }
    return newpagemap;
}

void Pagemap::deleteThis()
{
    lockit(this->lock);
    for (auto range : this->ranges)
    {
        this->munmap(reinterpret_cast<void*>(range->base), range->length);
    }
    delete this;
}

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

PDEntry *Pagemap::virt2pte(uint64_t vaddr, bool allocate, bool hugepages)
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
    if (hugepages) return &pml2->entries[pml2_entry];

    pml1 = get_next_lvl(pml2, pml2_entry, allocate);
    return &pml1->entries[pml1_entry];
}

void Pagemap::mapMem(uint64_t vaddr, uint64_t paddr, uint64_t flags, bool hugepages)
{
    lockit(this->lock);

    PDEntry *pml_entry = this->virt2pte(vaddr, true, hugepages);
    if (pml_entry == nullptr)
    {
        error("VMM: Could not get page map entry!");
        return;
    }

    pml_entry->setAddr(paddr >> 12);
    pml_entry->setflags(flags | (hugepages ? LargerPages : 0), true);
}

void Pagemap::mapMemRange(uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t flags, bool hugepages)
{
    for (size_t i = 0; i < size; i += (hugepages ? large_page_size : page_size))
    {
        this->mapMem(vaddr + i, paddr + i, flags, hugepages);
    }
}

bool Pagemap::remapMem(uint64_t vaddr_old, uint64_t vaddr_new, uint64_t flags)
{
    this->lock.lock();

    PDEntry *pml1_entry = this->virt2pte(vaddr_old, false);
    if (pml1_entry == nullptr)
    {
        error("VMM: Could not get page map entry!");
        this->lock.unlock();
        return false;
    }

    uint64_t paddr = pml1_entry->getAddr() << 12;
    pml1_entry->value = 0;
    invlpg(vaddr_old);
    this->lock.unlock();

    this->mapMem(vaddr_new, paddr, flags);
    return true;
}

bool Pagemap::unmapMem(uint64_t vaddr, bool hugepages)
{
    lockit(this->lock);

    PDEntry *pml_entry = this->virt2pte(vaddr, false, hugepages);
    if (pml_entry == nullptr)
    {
        error("VMM: Could not get page map entry!");
        return false;
    }

    pml_entry->value = 0;
    invlpg(vaddr);
    return true;
}

void Pagemap::unmapMemRange(uint64_t vaddr, uint64_t size, bool hugepages)
{
    for (size_t i = 0; i < size; i += (hugepages ? large_page_size : page_size))
    {
        this->unmapMem(vaddr + i);
    }
}

void Pagemap::switchTo()
{
    write_cr(3, reinterpret_cast<uint64_t>(this->TOPLVL));
}

void Pagemap::save()
{
    this->TOPLVL = reinterpret_cast<PTable*>(read_cr(3));
}

Pagemap *newPagemap()
{
    Pagemap *pagemap = new Pagemap;

    if (kernel_pagemap == nullptr)
    {
        pagemap->TOPLVL = reinterpret_cast<PTable*>(read_cr(3));
        return pagemap;
    }

    pagemap->TOPLVL = static_cast<PTable*>(pmm::alloc());

    PTable *toplvl = reinterpret_cast<PTable*>(reinterpret_cast<uint64_t>(pagemap->TOPLVL) + hhdm_offset);
    PTable *kerenltoplvl = reinterpret_cast<PTable*>(reinterpret_cast<uint64_t>(kernel_pagemap->TOPLVL) + hhdm_offset);
    for (size_t i = 0; i < 512; i++) toplvl[i] = kerenltoplvl[i];

    return pagemap;

    // Pagemap *pagemap = new Pagemap;
    // pagemap->TOPLVL = new PTable;

    // if (kernel_pagemap)
    // {
    //     PTable *toplvl = reinterpret_cast<PTable*>(reinterpret_cast<uint64_t>(pagemap->TOPLVL) + hhdm_offset);
    //     PTable *kerenltoplvl = reinterpret_cast<PTable*>(reinterpret_cast<uint64_t>(kernel_pagemap->TOPLVL) + hhdm_offset);
    //     for (size_t i = 256; i < 512; i++) toplvl[i] = kerenltoplvl[i];
    // }

    // for (uint64_t i = 0; i < 0x100000000; i += large_page_size)
    // {
    //     pagemap->mapMem(i, i, Present | ReadWrite | UserSuper, true);
    //     pagemap->mapMem(i + hhdm_offset, i, Present | ReadWrite | UserSuper, true);
    // }

    // for (size_t i = 0; i < memmap_request.response->entry_count; i++)
    // {
    //     limine_memmap_entry *mmap = memmap_request.response->entries[i];

    //     uint64_t base = ALIGN_DOWN(mmap->base, page_size);
    //     uint64_t top = ALIGN_UP(mmap->base + mmap->length, page_size);
    //     if (top < 0x100000000) continue;

    //     for (uint64_t t = base; t < top; t += page_size)
    //     {
    //         if (t < 0x100000000) continue;
    //         pagemap->mapMem(t, t, Present | ReadWrite | UserSuper);
    //         pagemap->mapMem(t + hhdm_offset, t, Present | ReadWrite | UserSuper);
    //     }
    // }

    // return pagemap;
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
    kernel_pagemap->switchTo();

    serial::newline();
    initialised = true;
}
}