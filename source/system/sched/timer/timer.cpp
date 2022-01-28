// Copyright (C) 2021-2022  ilobilo

#include <system/sched/timer/timer.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <lib/log.hpp>

namespace kernel::system::sched::timer {

void sleep(uint64_t sec)
{
    if (hpet::initialised) hpet::sleep(sec);
    else if (pit::initialised) pit::sleep(sec);
    else rtc::sleep(sec);
}

void msleep(uint64_t msec)
{
    if (hpet::initialised) hpet::msleep(msec);
    else if (pit::initialised) pit::msleep(msec);
    else
    {
        warn("HPET or PIT has not been initialised!");
        warn("Using RTC!");
        rtc::sleep(msec / 100);
    }
}

void usleep(uint64_t us)
{
    if (hpet::initialised) hpet::usleep(us);
    else if (pit::initialised)
    {
        warn("HPET has not been initialised!");
        warn("Using PIT!");
        pit::msleep(US2MS(us));
    }
    else
    {
        warn("HPET or PIT has not been initialised!");
        warn("Using RTC!");
        rtc::sleep(US2SEC(us));
    }
}
}