// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/cpu/pic/pic.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/acpi/acpi.hpp>
#include <lai/helpers/sci.h>
#include <lai/helpers/pm.h>
#include <lib/mmio.hpp>
#include <lib/msr.hpp>
#include <lib/io.hpp>
#include <main.hpp>
#include <cpuid.h>

using namespace kernel::drivers::display;

namespace kernel::system::cpu::apic {

bool initialised = false;
static bool x2apic = false;

static inline uint32_t reg2x2apic(uint32_t reg)
{
    uint32_t x2apic_reg = 0;
    if (reg == 0x310) x2apic_reg = 0x30;
    else x2apic_reg = reg >> 4;
    return x2apic_reg + 0x800;
}

uint32_t lapic_read(uint32_t reg)
{
    if (x2apic) return rdmsr(reg2x2apic(reg));
    return mmind((void*)(acpi::lapic_addr + reg));
}

void lapic_write(uint32_t reg, uint32_t value)
{
    if (x2apic) wrmsr(reg2x2apic(reg), value);
    else mmoutd((void*)(acpi::lapic_addr + reg), value);
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
        if (c & (1 << 21))
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
    
    for (size_t i = 0; i < acpi::nmis.size(); i++)
    {
        acpi::MADTNmi *nmi = acpi::nmis[i];
        lapic_set_nmi(2, processor_id, nmi->processor, nmi->flags, nmi->lint);
    }
}

uint32_t ioapic_read(uintptr_t ioapic_address, size_t reg)
{
    mmoutd((void*)ioapic_address, reg & 0xFF);
    return mmind((void*)(ioapic_address + 16));
}

void ioapic_write(uintptr_t ioapic_address, size_t reg, uint32_t data)
{
    mmoutd((void*)ioapic_address, reg & 0xFF);
    mmoutd((void*)(ioapic_address + 16), data);
}

static uint32_t get_gsi_count(uintptr_t ioapic_address)
{
    return (ioapic_read(ioapic_address, 1) & 0xFF0000) >> 16;
}

static acpi::MADTIOApic *get_ioapic_by_gsi(uint32_t gsi)
{
    for (size_t i = 0; i < acpi::ioapics.size(); i++)
    {
        acpi::MADTIOApic *ioapic = acpi::ioapics[i];
        if (ioapic->gsib <= gsi && ioapic->gsib + get_gsi_count(ioapic->addr) > gsi) return ioapic;
    }
    return NULL;
}

void ioapic_redirect_gsi(uint32_t gsi, uint8_t vec, uint16_t flags)
{
    size_t io_apic = get_ioapic_by_gsi(gsi)->addr;

    uint32_t low_index = 0x10 + (gsi - get_ioapic_by_gsi(gsi)->gsib) * 2;
    uint32_t high_index = low_index + 1;

    uint32_t high = ioapic_read(io_apic, high_index);

    high &= ~0xFF000000;
    high |= ioapic_read(io_apic, 0) << 24;
    ioapic_write(io_apic, high_index, high);

    uint32_t low = ioapic_read(io_apic, low_index);

    low &= ~(1 << 16);
    low &= ~(1 << 11);
    low &= ~0x700;
    low &= ~0xFF;
    low |= vec;
    if (flags & 2) low |= 1 << 13;
    if (flags & 8) low |= 1 << 15;

    ioapic_write(io_apic, low_index, low);
}

void ioapic_redirect_irq(uint32_t irq, uint8_t vect)
{
    for (size_t i = 0; i < acpi::isos.size(); i++)
    {
        if (acpi::isos[i]->irq_source == irq)
        {
            ioapic_redirect_gsi(acpi::isos[i]->gsi, vect, acpi::isos[i]->flags);
            return;
        }
    }

    ioapic_redirect_gsi(irq, vect, 0);
}

void apic_send_ipi(uint32_t lapic_id, uint32_t flags)
{
    if (x2apic) wrmsr(0x830, ((uint64_t)lapic_id << 32) | flags);
    else
    {
        lapic_write(0x310, (lapic_id << 24));
        lapic_write(0x300, flags);
    }
}

void eoi()
{
    lapic_write(0xB0, 0);
}

static void SCI_Handler(idt::registers_t *)
{
    serial::info("test sci");
    uint16_t event = lai_get_sci_event();
    if (event & ACPI_POWER_BUTTON) lai_enter_sleep(5);
}

void init()
{
    serial::info("Initialising APIC");

    if (initialised)
    {
        serial::warn("APIC has already been initialised!\n");
        return;
    }

    if (!acpi::madt || !acpi::madthdr)
    {
        serial::err("MADT table could not be found!\n");
        return;
    }

    pic::disable();
    lapic_init(acpi::lapics[0]->processor_id);

    idt::register_interrupt_handler(acpi::fadthdr->SCI_Interrupt + 32, SCI_Handler);

    serial::newline();
    initialised = true;
}
}