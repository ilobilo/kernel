// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/acpi/acpi.hpp>
#include <lib/mmio.hpp>

using namespace kernel::drivers::display;

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
    if (!initialised)
    {
        if (pit::initialised) pit::msleep(us / 10000);
        else rtc::sleep(us / 1000000);
        return;
    }
    uint64_t target = counter() + (us * 1000000000) / clk;
    while (counter() < target);
}

void msleep(uint64_t msec)
{
    usleep(HPETMSECS(msec));
}

void sleep(uint64_t sec)
{
    usleep(HPETSECS(sec));
}

void init()
{
    serial::info("Initialising HPET");

    if (initialised)
    {
        serial::warn("HPET has already been initialised!\n");
        return;
    }
    if (!acpi::hpethdr)
    {
        serial::err("HPET table not found!\n");
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