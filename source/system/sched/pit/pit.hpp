// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>

namespace kernel::system::sched::pit {

#define PIT_DEF_FREQ 100

#define MS2PIT(ms) (100 / (ms))
#define PIT2MS(freq) (100 / (freq))

extern bool initialised;
extern bool schedule;
extern uint64_t frequency;

void sleep(uint64_t sec);
void msleep(uint64_t msec);
uint64_t get_tick();

void setfreq(uint64_t freq = PIT_DEF_FREQ);
uint64_t getfreq();
void resetfreq();

void init(uint64_t freq = PIT_DEF_FREQ);
}