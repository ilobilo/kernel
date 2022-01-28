// Copyright (C) 2021-2022  ilobilo

#include <system/sched/pit/pit.hpp>
#include <lib/io.hpp>

using namespace kernel::system::sched;

namespace kernel::drivers::audio::pcspk {

void play(uint64_t freq)
{
    uint64_t divisor = 1193180 / freq;

    outb(0x43, 0xB6);

    uint8_t l = static_cast<uint8_t>(divisor);
    uint8_t h = static_cast<uint8_t>(divisor >> 8);

    outb(0x42, l);
    outb(0x42, h);

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