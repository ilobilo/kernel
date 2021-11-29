// Copyright (C) 2021  ilobilo

#include <system/sched/pit/pit.hpp>
#include <lib/io.hpp>

using namespace kernel::system::sched;

namespace kernel::drivers::audio::pcspk {

void play(uint64_t freq)
{
    uint64_t div = 1193180 / freq;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)div);
    outb(0x42, (uint8_t)(div >> 8));

    outb(0x61, inb(0x61) | 0x3);
}

void stop()
{
    outb(0x61, inb(0x61) & 0xFC);
}

void beep(uint64_t freq, uint64_t msec)
{
    play(freq);
    pit::msleep(msec);
    stop();
}
}