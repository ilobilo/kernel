// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::system::sched::timer {

#define SECS2HPET(num) ((num) * 1000000)
#define MSECS2HPET(num) ((num) * 10000)
#define MICSECS2HPET(num) ((num) * 10)

#define HPET2SECS(num) ((num) / 1000000)
#define HPET2MSECS(num) ((num) / 10000)
#define HPET2MICSECS(num) ((num) / 10)

void sleep(uint64_t sec);
void msleep(uint64_t msec);
void usleep(uint64_t us);
}