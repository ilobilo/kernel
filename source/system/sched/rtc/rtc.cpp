#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <lib/io.hpp>


namespace kernel::system::sched::rtc {

int bcdtobin(int value)
{
    return (value >> 4) * 10 + (value & 15);
}

int year()
{
    outb(0x70, 0x09);
    return bcdtobin(inb(0x71));
}

int month()
{
    outb(0x70, 0x08);
    return bcdtobin(inb(0x71));
}

int day()
{
    outb(0x70, 0x07);
    return bcdtobin(inb(0x71));
}

int hour()
{
    outb(0x70, 0x04);
    return bcdtobin(inb(0x71));
}

int minute()
{
    outb(0x70, 0x02);
    return bcdtobin(inb(0x71));
}

int second()
{
    outb(0x70, 0x00);
    return bcdtobin(inb(0x71));
}

int time()
{
    return hour() * 3600 + minute() * 60 + second();
}

void sleep(int sec)
{
    int lastsec = time() + sec;
    while (lastsec != time());
}

char *getTime()
{
    static char time[30];
    sprintf(time, "20%d/%d/%d %d:%d:%d", year(), month(),
        day(), hour(),
        minute(), second());
    return time;
}
}