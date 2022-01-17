// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::system::sched::timer {

#define SEC2US(num) ((num) * 1000000)
#define MS2US(num) ((num) * 10000)
#define MICS2US(num) ((num) * 10)

#define US2SEC(num) ((num) / 1000000)
#define US2MS(num) ((num) / 10000)
#define US2MICS(num) ((num) / 10)

void sleep(uint64_t sec);
void msleep(uint64_t msec);
void usleep(uint64_t us);
}