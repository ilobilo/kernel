// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::drivers::net::rtl8139 {

extern bool initialised;
extern uint8_t MAC[];

void send(void *data, uint64_t length);
void recive();

void reset();
void activate();

void init();
}