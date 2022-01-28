// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/acpi/acpi.hpp>
#include <kernel/kernel.hpp>
#include <lib/math.hpp>
#include <lib/io.hpp>

namespace kernel::system::sched::rtc {

int bcdtobin(int value)
{
    return (value >> 4) * 10 + (value & 15);
}

uint64_t century()
{
    if (acpi::fadthdr && acpi::fadthdr->Century == 0) return 20;
    outb(0x70, 0x32);
    return bcdtobin(inb(0x71));
}

uint64_t year()
{
    outb(0x70, 0x09);
    return bcdtobin(inb(0x71));
}

uint64_t month()
{
    outb(0x70, 0x08);
    return bcdtobin(inb(0x71));
}

uint64_t day()
{
    outb(0x70, 0x07);
    return bcdtobin(inb(0x71));
}

uint64_t hour()
{
    outb(0x70, 0x04);
    return bcdtobin(inb(0x71));
}

uint64_t minute()
{
    outb(0x70, 0x02);
    return bcdtobin(inb(0x71));
}

uint64_t second()
{
    outb(0x70, 0x00);
    return bcdtobin(inb(0x71));
}

uint64_t time()
{
    return hour() * 3600 + minute() * 60 + second();
}

uint64_t epoch()
{
    uint64_t seconds = second(), minutes = minute(), hours = hour(), days = day(), months = month(), years = year(), centuries = century();

    uint64_t jdn_current = jdn(days, months, centuries * 100 + years);
    uint64_t jdn_1970 = jdn(1, 1, 1970);
    uint64_t diff = jdn_current - jdn_1970;

    return (diff * (60 * 60 * 24)) + hours * 3600 + minutes * 60 + seconds;
}

uint64_t seconds_since_boot()
{
    return epoch() - epoch_tag->epoch;
}

void sleep(uint64_t sec)
{
    uint64_t lastsec = time() + sec;
    while (lastsec != time());
}

char timestr[30];
char *getTime()
{
    sprintf(timestr, "%.4ld/%.2ld/%.2ld %.2ld:%.2ld:%.2ld", century() * 100 + year(), month(),
        day(), hour(),
        minute(), second());
    return timestr;
}
}