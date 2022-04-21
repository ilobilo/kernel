// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <system/vfs/vfs.hpp>
#include <kernel/kernel.hpp>
#include <lib/math.hpp>
#include <cstdint>
#include <elf.h>

using namespace kernel::system::mm;
using namespace kernel::system;

struct Auxval
{
    uint64_t entry;
    uint64_t phdr;
    uint64_t phent;
    uint64_t phnum;
};

static inline auto elf_load(vmm::Pagemap *pagemap, vfs::resource_t *res, uint64_t base)
{
    struct ret { Auxval auxval; std::string ld_path; };
    ret null { Auxval { 0, 0, 0, 0 }, "" };

    Elf64_Ehdr *header = new Elf64_Ehdr;
    res->read(nullptr, reinterpret_cast<uint8_t*>(header), 0, sizeof(Elf64_Ehdr));

    if (memcmp(header->e_ident, ELFMAG, 4))
    {
        error("ELF: Invalid magic!");
        delete header;
        return null;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS64
        || header->e_ident[EI_DATA] != ELFDATA2LSB
        || header->e_ident[EI_OSABI] != ELFOSABI_SYSV
        || header->e_machine != R_IA64_PLTOFF64MSB)
    {
        error("ELF: Unsupported ELF file!");
        delete header;
        return null;
    }

    Auxval auxval
    {
        .entry = base + header->e_entry,
        .phdr = 0,
        .phent = sizeof(Elf64_Phdr),
        .phnum = header->e_phnum
    };

    std::string ld_path("");
    for (size_t i = 0; i < header->e_phnum; i++)
    {
        Elf64_Phdr *phdr = new Elf64_Phdr;
        res->read(nullptr, reinterpret_cast<uint8_t*>(phdr), header->e_phoff + sizeof(Elf64_Phdr) * i, sizeof(Elf64_Phdr));

        switch (phdr->p_type)
        {
            case PT_INTERP:
                ld_path.resize(phdr->p_filesz);
                res->read(nullptr, reinterpret_cast<uint8_t*>(ld_path.data()), phdr->p_offset, phdr->p_filesz);
                break;
            case PT_PHDR:
                auxval.phdr = base + phdr->p_vaddr;
                break;
            case PT_LOAD:
            {
                uint64_t misalign = (phdr->p_vaddr & (vmm::page_size - 1));
                uint64_t pages = DIV_ROUNDUP(misalign + phdr->p_memsz, vmm::page_size);
                void *addr = pmm::alloc(pages);
                if (addr == nullptr)
                {
                    error("ELF: Could not allocate memory!");
                    delete header;
                    delete phdr;
                    return null;
                }

                uint64_t vaddr = base + phdr->p_vaddr;
                uint64_t paddr = reinterpret_cast<uint64_t>(addr);

                pagemap->mapRange(vaddr, paddr, pages * vmm::page_size, vmm::ProtRead | vmm::ProtExec | (phdr->p_flags & PF_W ? vmm::ProtWrite : 0), vmm::MapAnon);

                uint8_t *buffer = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(addr) + misalign + hhdm_offset);
                res->read(0, buffer, phdr->p_offset, phdr->p_filesz);
                break;
            }
        }
        delete phdr;
    }
    delete header;
    return ret { auxval, ld_path };
}