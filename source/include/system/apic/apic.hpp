// Copyright (C) 2021  ilobilo

#pragma once

namespace kernel::system::apic {

#define CPUID_X2APIC (1 << 21)

extern bool initialised;

void lapic_init(uint8_t processor_id);

void eoi();

void init();
}