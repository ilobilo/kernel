// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>

namespace kernel::drivers::fs::initrd {

extern bool initialised;
void init(uint64_t module);
}