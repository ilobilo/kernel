// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::system::sched::pit {

extern bool initialised;
extern uint64_t frequency;

void sleep(uint64_t sec);
void msleep(uint64_t msec);
uint64_t get_tick();

void setfreq(uint64_t freq = 100);
void resetfreq();

void init(uint64_t freq = 100);
}