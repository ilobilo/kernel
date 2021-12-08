// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

namespace kernel::system::sched::hpet {

#define HPETSECS(num) ((num) * 1000000)
#define HPETMSECS(num) ((num) * 10000)
#define HPETMICSECS(num) ((num) * 10)

struct HPET
{
    uint64_t general_capabilities;
    uint64_t reserved;
    uint64_t general_configuration;
    uint64_t reserved2;
    uint64_t general_int_status;
    uint64_t reserved3;
    uint64_t reserved4[24];
    uint64_t main_counter_value;
    uint64_t reserved5;
};

extern bool initialised;
extern HPET *hpet;

uint64_t counter();

void usleep(uint64_t us);
void msleep(uint64_t msec);
void sleep(uint64_t sec);

void init();
}