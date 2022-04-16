// Copyright (C) 2021-2022  ilobilo

#include <system/cpu/pic/pic.hpp>
#include <lib/io.hpp>

namespace kernel::system::cpu::pic {

void eoi(uint64_t int_no)
{
    if (int_no >= 40) outb(pic::PIC2_COMMAND, pic::PIC_EOI);
    outb(pic::PIC1_COMMAND, pic::PIC_EOI);
}

void disable()
{
    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);
}

void init()
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}
}