// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/trace/trace.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/alloc.hpp>
#include <lib/log.hpp>
#include <elf.h>

using namespace kernel::drivers::display;

namespace kernel::system::trace {

symtable_t *symbol_table;
size_t entries = 0;

symtable_t lookup(uint64_t addr)
{
    symtable_t result{0, "<unknown>"};
    for (size_t i = 0; i < entries; i++)
    {
        if (symbol_table[i].addr <= addr && symbol_table[i].addr > result.addr)
        {
            result = symbol_table[i];
        }
    }
    return result;
}

bool backtrace(uint64_t addr, size_t i, bool terminal)
{
    symtable_t symtable = lookup(addr);

    if (!strcmp(symtable.name, "<unknown>") || symtable.addr == 0) return false;
    error("#%zu 0x%lX \t%s", i, symtable.addr, symtable.name);
    if (terminal) printf("\n[\033[31mPANIC\033[0m] #%zu 0x%lX \t%s", i, symtable.addr, symtable.name);

    return true;
}

void trace(bool terminal)
{
    static stackframe_t *sf = nullptr;
    sf = reinterpret_cast<stackframe_t*>(__builtin_frame_address(0));

    error("Stack trace:");
    if (terminal) printf("\n[\033[31mPANIC\033[0m] Stack trace:");

    for (size_t i = 0; i < 15 && sf; i++)
    {
        if (!backtrace(sf->rip, i, terminal)) break;
        sf = sf->frame;
    }
}

void init()
{
    Elf64_Ehdr *header = reinterpret_cast<Elf64_Ehdr*>(kfilev2_tag->kernel_file);
    Elf64_Shdr *sections = reinterpret_cast<Elf64_Shdr*>(kfilev2_tag->kernel_file + header->e_shoff);
    Elf64_Sym *symtab;
    char *strtab;

    for (size_t i = 0; i < header->e_shnum; i++)
    {
        switch (sections[i].sh_type)
        {
            case SHT_SYMTAB:
                symtab = reinterpret_cast<Elf64_Sym*>(kfilev2_tag->kernel_file + sections[i].sh_offset);
                entries = sections[i].sh_size / sections[i].sh_entsize;
                break;
            case SHT_STRTAB:
                strtab = reinterpret_cast<char*>(kfilev2_tag->kernel_file + sections[i].sh_offset);
                break;
        }
    }

    size_t j, min_idx;
    for (size_t i = 0; i < entries - 1; i++)
    {
        min_idx = i;
        for (j = i + 1; j < entries; j++) if (symtab[j].st_value < symtab[min_idx].st_value) min_idx = j;

        Elf64_Sym temp = symtab[min_idx];
        symtab[min_idx] = symtab[i];
        symtab[i] = temp;
    }

    while (symtab[0].st_value == 0)
    {
        symtab++;
        entries--;
    }

    symbol_table = static_cast<symtable_t*>(calloc(entries, sizeof(symtable_t)));

    for (size_t i = 0, entriesbck = entries; i < entriesbck; i++)
    {
        symbol_table[i].addr = symtab[i].st_value;
        symbol_table[i].name = &strtab[symtab[i].st_name];
    }
}
}