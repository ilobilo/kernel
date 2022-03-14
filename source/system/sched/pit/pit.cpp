// Copyright (C) 2021-2022  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>

using namespace kernel::system::cpu;

namespace kernel::system::sched::pit {

bool initialised = false;
bool schedule = false;
uint64_t tick = 0;
uint64_t frequency = PIT_DEF_FREQ;
new_lock(pit_lock)

void sleep(uint64_t sec)
{
    uint64_t start = tick;
    while (tick < start + sec * frequency);
}

void msleep(uint64_t msec)
{
    uint64_t start = tick;
    while (tick < start + msec * (frequency / 100));
}

uint64_t get_tick()
{
    return tick;
}

static void PIT_Handler(registers_t *regs)
{
    tick++;
    if (schedule) scheduler::switchTask(regs);
}

void setfreq(uint64_t freq)
{
    lockit(pit_lock);

    if (freq < 19) freq = 19;
    frequency = freq;
    uint64_t divisor = 1193180 / frequency;

    outb(0x43, 0x36);

    uint8_t l = static_cast<uint8_t>(divisor);
    uint8_t h = static_cast<uint8_t>(divisor >> 8);

    outb(0x40, l);
    outb(0x40, h);
}

uint64_t getfreq()
{
    lockit(pit_lock);

    uint64_t freq = 0;
    outb(0x43, 0b0000000);
    freq = inb(0x40);
    freq |= inb(0x40) << 8;
    freq = 1193180 / freq;

    return freq;
}

void resetfreq()
{
    setfreq(PIT_DEF_FREQ);
}

void init(uint64_t freq)
{
    log("Initialising PIT");

    if (initialised)
    {
        warn("PIT has already been initialised!\n");
        return;
    }

    setfreq(freq);
    idt::register_interrupt_handler(idt::IRQ0, PIT_Handler, true);

    serial::newline();
    initialised = true;
}
}