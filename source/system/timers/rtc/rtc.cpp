#include <drivers/display/terminal/terminal.hpp>
#include <system/timers/rtc/rtc.hpp>
#include <include/io.hpp>

uint8_t bcdtobin(uint8_t value)
{
    return (value >> 4) * 10 + (value & 15);
}

uint8_t year()
{
    outb(0x70, 0x09);
    return bcdtobin(inb(0x71));
}

uint8_t month()
{
    outb(0x70, 0x08);
    return bcdtobin(inb(0x71));
}

uint8_t day()
{
    outb(0x70, 0x07);
    return bcdtobin(inb(0x71));
}

uint8_t hour()
{
    outb(0x70, 0x04);
    return bcdtobin(inb(0x71));
}

uint8_t minute()
{
    outb(0x70, 0x02);
    return bcdtobin(inb(0x71));
}

uint8_t second()
{
    outb(0x70, 0x00);
    return bcdtobin(inb(0x71));
}

void RTC_GetTime()
{
    printf("20%d/%d/%d %d:%d:%d\n", year(), month(),
        day(), hour(),
        minute(), second());
}