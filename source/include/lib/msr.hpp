// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t value);