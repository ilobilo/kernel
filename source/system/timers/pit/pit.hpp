#pragma once

#include <stdint.h>

void sleep(long sec);

uint64_t get_tick();

void PIT_init();
