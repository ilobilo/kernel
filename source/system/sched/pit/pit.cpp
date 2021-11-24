// Copyright (C) 2021  ilobilo

#include <drivers/display/serial/serial.hpp>
#include <system/sched/hpet/hpet.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::cpu;

namespace kernel::system::sched::pit {

bool initialised = false;
uint64_t frequency = 100;
uint64_t freqbck = 100;
uint64_t tick = 0;

void sleep(uint64_t sec)
{
    if (hpet::initialised)
    {
        hpet::sleep(sec);
        return;
    }
    if (!initialised) rtc::sleep(sec);

    long start = tick;
    while (tick < start + sec * frequency);
}

void msleep(uint64_t msec)
{
    if (hpet::initialised)
    {
        hpet::msleep(msec);
        return;
    }
    if (!initialised) rtc::sleep(msec / 100);

    long start = tick;
    while (tick < start + msec * (frequency / 100));
}

uint64_t get_tick()
{
    return tick;
}

static void PIT_Handler(idt::registers_t *)
{
    tick++;
}

void setfreq(uint64_t freq)
{
    frequency = freq;
    uint64_t divisor = 1193180 / freq;

    outb(0x43, 0x36);

    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xFF );

    outb(0x40, l);
    outb(0x40, h);
}

void resetfreq()
{
    setfreq(freqbck);
}

void init(uint64_t freq)
{
    serial::info("Initialising PIT");

    if (initialised)
    {
        serial::info("PIT has already been initialised!\n");
        return;
    }

    freqbck = freq;
    setfreq(freq);

    register_interrupt_handler(idt::IRQS::IRQ0, PIT_Handler);

    serial::newline();
    initialised = true;
}
}