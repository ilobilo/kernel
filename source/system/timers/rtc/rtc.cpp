#include <drivers/display/terminal/terminal.hpp>
#include <system/timers/rtc/rtc.hpp>
#include <include/io.hpp>

int bcdtobin(int value)
{
    return (value >> 4) * 10 + (value & 15);
}

int RTC_year()
{
    outb(0x70, 0x09);
    return bcdtobin(inb(0x71));
}

int RTC_month()
{
    outb(0x70, 0x08);
    return bcdtobin(inb(0x71));
}

int RTC_day()
{
    outb(0x70, 0x07);
    return bcdtobin(inb(0x71));
}

int RTC_hour()
{
    outb(0x70, 0x04);
    return bcdtobin(inb(0x71));
}

int RTC_minute()
{
    outb(0x70, 0x02);
    return bcdtobin(inb(0x71));
}

int RTC_second()
{
    outb(0x70, 0x00);
    return bcdtobin(inb(0x71));
}

int RTC_time()
{
    return RTC_hour() * 3600 + RTC_minute() * 60 + RTC_second();
}

void RTC_sleep(int sec)
{
    int lastsec = RTC_time() + sec;
    while (lastsec != RTC_time());
}

char* RTC_GetTime()
{
    static char time[30];
    sprintf(time, "20%d/%d/%d %d:%d:%d", RTC_year(), RTC_month(),
        RTC_day(), RTC_hour(),
        RTC_minute(), RTC_second());
    return time;
}