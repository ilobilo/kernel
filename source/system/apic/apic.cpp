// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/acpi/madt.hpp>
#include <system/apic/apic.hpp>
#include <lib/mmio.hpp>
#include <lib/msr.hpp>
#include <lib/io.hpp>
#include <main.hpp>
#include <cpuid.h>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

namespace kernel::system::apic {

bool initialised = false;

static uintptr_t lapic_addr;
static bool x2apic = false;

static inline uint32_t reg2x2apic(uint32_t reg)
{
    uint32_t x2apic_reg = 0;
    if (reg == 0x310) x2apic_reg = 0x30;
    else x2apic_reg = reg >> 4;
    return x2apic_reg + 0x800;
}

static uint32_t lapic_read(uint32_t reg)
{
    if (x2apic) return rdmsr(reg2x2apic(reg));
    return mmind((void*)(lapic_addr + reg));
}

static void lapic_write(uint32_t reg, uint32_t value)
{
    if (x2apic) wrmsr(reg2x2apic(reg), value);
    else mmoutd((void*)(lapic_addr + reg), value);
}

static void lapic_set_nmi(uint8_t vec, uint8_t current_processor_id, uint8_t processor_id, uint16_t flags, uint8_t lint)
{
    if (processor_id != 0xFF) if (current_processor_id != processor_id) return;
    
    uint32_t nmi = 0x400 | vec;
    if (flags & 2) nmi |= 1 << 13;
    
    if (flags & 8) nmi |= 1 << 15;
    
    if (lint == 0) lapic_write(0x350, nmi);
    else if (lint == 1) lapic_write(0x360, nmi);
}

void lapic_init(uint8_t processor_id)
{
    uint64_t apic_msr = rdmsr(0x1B);
    apic_msr |= 1 << 11;
    uint32_t a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid(1, &a, &b, &c, &d))
    {
        if (c & CPUID_X2APIC)
        {
            x2apic = true;
            apic_msr |= 1 << 10;
        }
    }
    
    wrmsr(0x1B, apic_msr);
    lapic_write(0x80, 0);
    lapic_write(0xF0, lapic_read(0xF0) | 0x100);
    if (!x2apic)
    {
        lapic_write(0xE0, 0xF0000000);
        lapic_write(0xD0, lapic_read(0x20));
    }
    
    for (size_t i = 0; i < madt::MADTNmis.size(); i++)
    {
        madt::MADTnmi *nmi = madt::MADTNmis[i];
        lapic_set_nmi(2, processor_id, nmi->processor, nmi->flags, nmi->lint);
    }
}

void eoi()
{
    lapic_write(0xB0, 0);
}

void init()
{
    serial::info("Initialising APIC");

    if (initialised)
    {
        serial::warn("APIC has already been initialised!\n");
        return;
    }

    lapic_addr = madt::getLapicAddr();
    lapic_init(smp_tag->bsp_lapic_id);

    serial::newline();
    initialised = true;
}
}