// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kernel::system::cpu::gdt {

struct [[gnu::packed]] GDTDescriptor
{
    uint16_t Size;
    uint64_t Offset;
};

struct [[gnu::packed]] GDTEntry
{
    uint16_t Limit0;
    uint16_t Base0;
    uint8_t Base1;
    uint8_t Access;
    uint8_t Granularity;
    uint8_t Base2;
};

struct [[gnu::packed]] TSSEntry
{
    uint16_t Length;
    uint16_t Base0;
    uint8_t  Base1;
    uint8_t  Flags1;
    uint8_t  Flags2;
    uint8_t  Base2;
    uint32_t Base3;
    uint32_t Reserved;
};

struct [[gnu::packed, gnu::aligned(0x1000)]] GDT
{
    GDTEntry Null;
    GDTEntry _16BitCode;
    GDTEntry _16BitData;
    GDTEntry _32BitCode;
    GDTEntry _32BitData;
    GDTEntry _64BitCode;
    GDTEntry _64BitData;
    GDTEntry UserData;
    GDTEntry UserCode;
    TSSEntry Tss;
};

struct [[gnu::packed]] TSS
{
    uint32_t Reserved0;
    uint64_t RSP[3];
    uint64_t Reserved1;
    uint64_t IST[7];
    uint64_t Reserved2;
    uint16_t Reserved3;
    uint16_t IOPBOffset;
};

extern GDT DefaultGDT;
extern bool initialised;
extern TSS *tss;

extern "C" void LoadGDT(GDTDescriptor *gdtDescriptor);
extern "C" void LoadTSS();

void reloadall(size_t cpu);
void reloadgdt();
void reloadtss();
void init();
}