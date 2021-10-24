#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/drawing/drawing.hpp>
#include <drivers/devices/ps2/mouse/mouse.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::cpu;

namespace kernel::drivers::ps2::mouse {

bool initialised = false;;

uint8_t cycle = 0;
uint8_t packet[4];
bool packetready = false;
math::point mousepos;
math::point mouseposold;

uint32_t mousebordercol = 0xFFFFFF;
uint32_t mouseinsidecol = 0x2D2D2D;

uint8_t cursorborder[]
{
    0b10000000, 0b00000000,
    0b11000000, 0b00000000,
    0b10100000, 0b00000000,
    0b10010000, 0b00000000,
    0b10001000, 0b00000000,
    0b10000100, 0b00000000,
    0b10000010, 0b00000000,
    0b10000001, 0b00000000,
    0b10000000, 0b10000000,
    0b10000000, 0b01000000,
    0b10000000, 0b00100000,
    0b10000000, 0b00010000,
    0b10000001, 0b11110000,
    0b10001001, 0b00000000,
    0b10010100, 0b10000000,
    0b10100100, 0b10000000,
    0b11000010, 0b01000000,
    0b00000010, 0b01000000,
    0b00000001, 0b10000000,
};

uint8_t cursorinside[]
{
    0b10000000, 0b00000000,
    0b11000000, 0b00000000,
    0b11100000, 0b00000000,
    0b11110000, 0b00000000,
    0b11111000, 0b00000000,
    0b11111100, 0b00000000,
    0b11111110, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b10000000,
    0b11111111, 0b11000000,
    0b11111111, 0b11100000,
    0b11111111, 0b11110000,
    0b11111111, 0b11110000,
    0b11111111, 0b00000000,
    0b11110111, 0b10000000,
    0b11100111, 0b10000000,
    0b11000011, 0b11000000,
    0b00000011, 0b11000000,
    0b00000001, 0b10000000,
};

void mousewait()
{
    uint64_t timeout = 100000;
    while (timeout--) if ((io::inb(0x64) & 0b10) == 0) return;
}

void mousewait_input()
{
    uint64_t timeout = 100000;
    while (timeout--) if (io::inb(0x64) & 0b1) return;
}

void mousewrite(uint8_t value)
{
    mousewait();
    io::outb(0x64, 0xD4);
    mousewait();
    io::outb(0x60, value);
}

uint8_t mouseread()
{
    mousewait_input();
    return io::inb(0x60);
}

mousestate getmousestate()
{
    if (packet[0] & ps2_left) return ps2_left;
    else if (packet[0] & ps2_middle) return ps2_middle;
    else if (packet[0] & ps2_right) return ps2_right;
    return ps2_none;
}

void proccesspacket()
{
    if (!packetready) return;

    bool xmin, ymin, xover, yover;

    if (packet[0] & PS2_X_SIGN) xmin = true;
    else xmin = false;

    if (packet[0] & PS2_Y_SIGN) ymin = true;
    else ymin = false;

    if (packet[0] & PS2_X_OVER) xover = true;
    else xover = false;

    if (packet[0] & PS2_Y_OVER) yover = true;
    else yover = false;

    if (!xmin)
    {
        mousepos.X += packet[1];
        if (xover)
        {
            mousepos.X += 255;
        }
    }
    else
    {
        packet[1] = 256 - packet[1];
        mousepos.X -= packet[1];
        if (xover)
        {
            mousepos.X -= 255;
        }
    }

    if (!ymin)
    {
        mousepos.Y -= packet[2];
        if (yover)
        {
            mousepos.Y -= 255;
        }
    }
    else
    {
        packet[2] = 256 - packet[2];
        mousepos.Y += packet[2];
        if (yover)
        {
            mousepos.Y += 255;
        }
    }

    if (mousepos.X < 0) mousepos.X = 0;
    if (mousepos.X > drawing::frm_width - 1) mousepos.X = drawing::frm_width - 1;

    if (mousepos.Y < 0) mousepos.Y = 0;
    if (mousepos.Y > drawing::frm_height - 1) mousepos.Y = drawing::frm_height - 1;

    drawing::clearcursor(cursorinside, mouseposold);

    drawing::drawovercursor(cursorinside, mousepos, mouseinsidecol, true);
    drawing::drawovercursor(cursorborder, mousepos, mousebordercol, false);

    packetready = false;
    mouseposold = mousepos;
}

static void Mouse_Handler(idt::interrupt_registers *)
{
    uint8_t mousedata = io::inb(0x60);

    proccesspacket();

    static bool skip = true;
    if (skip)
    {
        skip = false;
        return;
    }

    switch (cycle)
    {
        case 0:
            if ((mousedata & 0b00001000) == 0) break;
            packet[0] = mousedata;
            cycle++;
            break;
        case 1:
            packet[1] = mousedata;
            cycle++;
            break;
        case 2:
            packet[2] = mousedata;
            packetready = true;
            cycle = 0;
            break;
    }
}

void init()
{
    serial::info("Initialising PS2 mouse");

    if (initialised)
    {
        serial::info("Mouse driver has already been initialised!\n");
        return;
    }

    if (!idt::initialised)
    {
        serial::info("IDT has not been initialised!");
        idt::init();
    }
    else serial::newline();

    asm volatile ("cli");

    io::outb(0x64, 0xA8);

    mousewait();
    io::outb(0x64, 0x20);
    mousewait_input();

    uint8_t status = io::inb(0x60);
    status |= 0b10;
    mousewait();

    io::outb(0x64, 0x60);
    mousewait();
    io::outb(0x60, status);

    mousewrite(0xF6);
    mouseread();
    mousewrite(0xF4);
    mouseread();

    asm volatile ("sti");

    register_interrupt_handler(idt::IRQS::IRQ12, Mouse_Handler);

    initialised = true;
}
}