// Copyright (C) 2021  ilobilo

#pragma once

#include <system/acpi/acpi.hpp>
#include <lib/vector.hpp>
#include <stdint.h>

namespace kernel::system::madt {

struct MADT
{
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct MADTlapic
{
    MADT madtHeader;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct MADTioapic
{
    MADT madtHeader;
    uint8_t apic_id;
    uint8_t reserved;
    uint32_t addr;
    uint32_t gsib;
} __attribute__((packed));

struct MADTiso
{
    MADT madtHeader;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed));

struct MADTnmi
{
    MADT madtHeader;
    uint8_t processor;
    uint16_t flags;
    uint8_t lint;
} __attribute__((packed));

extern bool initialised;

extern Vector<MADTlapic*> MADTLapics;
extern Vector<MADTioapic*> MADTIOApics;
extern Vector<MADTiso*> MADTIsos;
extern Vector<MADTnmi*> MADTNmis;

uintptr_t getLapicAddr();

void init();
}