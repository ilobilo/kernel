#pragma once

#include <stdint.h>

namespace kernel::system::sched::pit {

extern uint64_t frequency;

extern bool initialised;

void sleep(double sec);

uint64_t get_tick();

void setfreq(uint64_t freq = 100);

void init(uint64_t freq = 100);
}