#pragma once

#include <stdint.h>

extern uint64_t pit_frequency;

extern bool pit_initialised;

void PIT_sleep(double sec);

uint64_t get_tick();

void PIT_setfreq(uint64_t freq = 100);

void PIT_init(uint64_t freq = 100);