// Copyright (C) 2021-2022  ilobilo

#include <system/sched/timer/timer.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <system/acpi/acpi.hpp>
#include <lib/mmio.hpp>
#include <lib/log.hpp>

namespace kernel::system::sched::hpet {

bool initialised = false;
static uint32_t clk = 0;
HPET *hpet;

uint64_t counter()
{
    return mminq(&hpet->main_counter_value);
}

void usleep(uint64_t us)
{
    uint64_t target = counter() + (us * 1000000000) / clk;
    while (counter() < target);
}

void msleep(uint64_t msec)
{
    usleep(MS2MICS(msec));
}

void sleep(uint64_t sec)
{
    usleep(SEC2MICS(sec));
}

void init()
{
    log("Initialising HPET");

    if (initialised)
    {
        warn("HPET has already been initialised!\n");
        return;
    }
    if (!acpi::hpethdr)
    {
        error("HPET table not found!\n");
        return;
    }

    hpet = reinterpret_cast<HPET*>(acpi::hpethdr->address.Address);
    clk = hpet->general_capabilities >> 32;

    mmoutq(&hpet->general_configuration, 0);
    mmoutq(&hpet->main_counter_value, 0);
    mmoutq(&hpet->general_configuration, 1);

    serial::newline();
    initialised = true;
}
}