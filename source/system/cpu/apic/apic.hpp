// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kernel::system::cpu::apic {

enum events
{
    ACPI_TIMER = 0x0001,
    ACPI_BUSMASTER = 0x0010,
    ACPI_GLOBAL = 0x0020,
    ACPI_POWER_BUTTON = 0x0100,
    ACPI_SLEEP_BUTTON = 0x0200,
    ACPI_RTC_ALARM = 0x0400,
    ACPI_PCIE_WAKE = 0x4000,
    ACPI_WAKE = 0x8000
};

extern bool initialised;

uint32_t lapic_read(uint32_t reg);
void lapic_write(uint32_t reg, uint32_t value);

uint32_t ioapic_read(uintptr_t ioapic_address, size_t reg);
void ioapic_write(uintptr_t ioapic_address, size_t reg, uint32_t data);

void ioapic_redirect_gsi(uint32_t gsi, uint8_t vec, uint16_t flags);
void ioapic_redirect_irq(uint32_t irq, uint8_t vect);
void apic_send_ipi(uint32_t lapic_id, uint32_t flags);
void eoi();

void lapic_oneshot(uint8_t vector, uint64_t ms);
void lapic_periodic(uint8_t vector, uint64_t ms = 1);

void lapic_init(uint8_t processor_id);
void init();
}