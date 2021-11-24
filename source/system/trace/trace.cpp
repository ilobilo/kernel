// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/trace/trace.hpp>
#include <lib/string.hpp>
#include <main.hpp>
#include <elf.h>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

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

bool backtrace(uint64_t addr, size_t i)
{
    symtable_t symtable = lookup(addr);

    serial::err("#%zu 0x%lX \t%s", i, symtable.addr, symtable.name);

    if (!strcmp(symtable.name, "_start")) return false;
    return true;
}

void trace()
{
    static stackframe_t *sf;
    asm("movq %%rbp, %0" : "=r"(sf));
    sf = sf->frame->frame;

    serial::err("Stack trace:");

    for (size_t i = 0; i < 10 && sf; i++)
    {
        if (!backtrace(sf->rip, i)) break;
        sf = sf->frame;
    }
}

void init()
{
    Elf64_Ehdr *header = (Elf64_Ehdr*)kfilev2_tag->kernel_file;
    Elf64_Shdr *sections = (Elf64_Shdr*)((char*)kfilev2_tag->kernel_file + header->e_shoff);
    Elf64_Sym *symtab;
    char *strtab;

    for (size_t i = 0; i < header->e_shnum; i++)
    {
        switch (sections[i].sh_type)
        {
            case SHT_SYMTAB:
                symtab = (Elf64_Sym*)((char*)kfilev2_tag->kernel_file + sections[i].sh_offset);
                entries = sections[i].sh_size / sections[i].sh_entsize;
                break;
            case SHT_STRTAB:
                strtab = (char*)((char*)kfilev2_tag->kernel_file + sections[i].sh_offset);
                break;
        }
    }

    symbol_table = (symtable_t*)heap::calloc(entries, sizeof(symtable_t));

    for (size_t i = 0; i < entries; i++)
    {
        symbol_table[i].addr = symtab[i].st_value;
        symbol_table[i].name = &strtab[symtab[i].st_name];
    }
    
    size_t j, min_idx;
 
    for (size_t i = 0; i < entries - 1; i++)
    {
        min_idx = i;
        for (j = i + 1; j < entries; j++) if (symbol_table[j].addr < symbol_table[min_idx].addr) min_idx = j;

        symtable_t temp = symbol_table[min_idx];
        symbol_table[min_idx] = symbol_table[i];
        symbol_table[i] = temp;
    }
}
}