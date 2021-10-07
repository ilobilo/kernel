#pragma once

#include <stdint.h>

void PIT_sleep(double sec);

uint64_t get_tick();

void PIT_init();
