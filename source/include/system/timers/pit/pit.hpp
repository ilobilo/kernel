#pragma once

#include <stdint.h>

extern bool pit_initialised;

void PIT_sleep(double sec);

uint64_t get_tick();

void PIT_init();
