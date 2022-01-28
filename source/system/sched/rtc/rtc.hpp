// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::system::sched::rtc {

uint64_t century(), year(), month(), day(), hour(), minute(), second();

uint64_t time();
uint64_t epoch();
uint64_t seconds_since_boot();

void sleep(uint64_t sec);

char *getTime();
}