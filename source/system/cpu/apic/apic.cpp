// Copyright (C) 2021-2022  ilobilo

#include <system/sched/timer/timer.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/pic/pic.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/acpi/acpi.hpp>
#include <kernel/main.hpp>
#include <lib/mmio.hpp>
#include <lib/cpu.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>
#include <cpuid.h>

using namespace kernel::system::sched;

namespace kernel::system::cpu::apic {

bool initialised = false;
static bool x2apic = false;
static uint64_t ticks_in_1ms = 0;

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
    return mminl(reinterpret_cast<void*>(acpi::lapic_addr + reg));
}

void lapic_write(uint32_t reg, uint32_t value)
{
    if (x2apic) wrmsr(reg2x2apic(reg), value);
    else mmoutl(reinterpret_cast<void*>(acpi::lapic_addr + reg), value);
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
    uint64_t apic_msr = rdmsr(0x1B) | (1 << 11);
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
    mmoutl(reinterpret_cast<void*>(ioapic_address), reg & 0xFF);
    return mminl(reinterpret_cast<void*>(ioapic_address + 16));
}

void ioapic_write(uintptr_t ioapic_address, size_t reg, uint32_t data)
{
    mmoutl(reinterpret_cast<void*>(ioapic_address), reg & 0xFF);
    mmoutl(reinterpret_cast<void*>(ioapic_address + 16), data);
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
    return nullptr;
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
    if (x2apic) wrmsr(0x830, (static_cast<uint64_t>(lapic_id) << 32) | flags);
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

void lapic_timer_mask(bool masked)
{
    if (masked) lapic_write(0x320, lapic_read(0x320) | (1 << 0x10));
    else lapic_write(0x320, lapic_read(0x320) & ~(1 << 0x10));
}

void lapic_timer_init()
{
    if (ticks_in_1ms == 0)
    {
        lapic_write(0x3E0, 0x03);
        lapic_write(0x380, 0xFFFFFFFF);
        lapic_timer_mask(false);
        timer::msleep(1);
        lapic_timer_mask(true);
        ticks_in_1ms = 0xFFFFFFFF - lapic_read(0x390);
    }
}

new_lock(lapic_timer_lock);;
void lapic_oneshot(uint8_t vector, uint64_t ms)
{
    lockit(lapic_timer_lock);

    lapic_timer_init();
    lapic_timer_mask(true);
    lapic_write(0x3E0, 0x03);
    lapic_write(0x320, (((lapic_read(0x320) & ~(0x03 << 17)) | (0x00 << 17)) & 0xFFFFFF00) | vector);
    lapic_write(0x380, ticks_in_1ms * ms);
    lapic_timer_mask(false);
}

void lapic_periodic(uint8_t vector, uint64_t ms)
{
    lockit(lapic_timer_lock);

    lapic_timer_init();
    lapic_timer_mask(true);
    lapic_write(0x3E0, 0x03);
    lapic_write(0x320, (((lapic_read(0x320) & ~(0x03 << 17)) | (0x01 << 17)) & 0xFFFFFF00) | vector);
    lapic_write(0x380, ticks_in_1ms * ms);
    lapic_timer_mask(false);
}

uint16_t getSCIevent()
{
    uint16_t a = 0, b = 0;
    if (acpi::fadthdr->PM1aEventBlock)
    {
        a = inw(acpi::fadthdr->PM1aEventBlock);
        outw(acpi::fadthdr->PM1aEventBlock, a);
    }
    if (acpi::fadthdr->PM1bEventBlock)
    {
        b = inw(acpi::fadthdr->PM1bEventBlock);
        outw(acpi::fadthdr->PM1bEventBlock, b);
    }
    return a | b;
}

void setSCIevent(uint16_t value)
{
    uint16_t a = acpi::fadthdr->PM1aEventBlock + (acpi::fadthdr->PM1EventLength / 2);
    uint16_t b = acpi::fadthdr->PM1bEventBlock + (acpi::fadthdr->PM1EventLength / 2);

    if (acpi::fadthdr->PM1aEventBlock) outw(a, value);
    if (acpi::fadthdr->PM1bEventBlock) outw(b, value);
}

static void SCI_Handler(registers_t *)
{
    uint16_t event = getSCIevent();
    if (event & ACPI_POWER_BUTTON)
    {
        acpi::shutdown();
        timer::msleep(50);
        outw(0xB004, 0x2000);
        outw(0x604, 0x2000);
        outw(0x4004, 0x3400);
    }
}

void init()
{
    log("Initialising APIC");

    if (initialised)
    {
        warn("APIC has already been initialised!\n");
        return;
    }

    if (!acpi::madt || !acpi::madthdr)
    {
        error("Could not find MADT table!\n");
        return;
    }

    pic::disable();
    lapic_init(acpi::lapics[0]->processor_id);

    setSCIevent(ACPI_POWER_BUTTON | ACPI_SLEEP_BUTTON | ACPI_WAKE);
    getSCIevent();

    idt::register_interrupt_handler(acpi::fadthdr->SCI_Interrupt + 32, SCI_Handler, true);
    ioapic_redirect_irq(acpi::fadthdr->SCI_Interrupt, acpi::fadthdr->SCI_Interrupt + 32);

    // COM1
    ioapic_redirect_irq(4, 36);

    serial::newline();
    initialised = true;
}
}