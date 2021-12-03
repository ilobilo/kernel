// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kernel::system::cpu::apic {

extern bool initialised;

uint32_t lapic_read(uint32_t reg);
void lapic_write(uint32_t reg, uint32_t value);

uint32_t ioapic_read(uintptr_t ioapic_address, size_t reg);
void ioapic_write(uintptr_t ioapic_address, size_t reg, uint32_t data);

void ioapic_redirect_gsi(uint32_t gsi, uint8_t vec, uint16_t flags);
void ioapic_redirect_irq(uint32_t irq, uint8_t vect);
void apic_send_ipi(uint32_t lapic_id, uint32_t flags);
void eoi();

void lapic_init(uint8_t processor_id);
void init();
}