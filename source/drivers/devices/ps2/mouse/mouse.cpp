#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/drawing/drawing.hpp>
#include <drivers/devices/ps2/mouse/mouse.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/idt/idt.hpp>
#include <include/io.hpp>

uint8_t cycle = 0;
uint8_t packet[4];
bool packetready = false;
point mousepos;
point mouseposold;

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
    while (timeout--) if ((inb(0x64) & 0b10) == 0) return;
}

void mousewait_input()
{
    uint64_t timeout = 100000;
    while (timeout--) if (inb(0x64) & 0b1) return;
}

void mousewrite(uint8_t value)
{
    mousewait();
    outb(0x64, 0xD4);
    mousewait();
    outb(0x60, value);
}

uint8_t mouseread()
{
    mousewait_input();
    return inb(0x60);
}

mousestate get_mousestate()
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
    if (mousepos.X > frm_width - 1) mousepos.X = frm_width - 1;

    if (mousepos.Y < 0) mousepos.Y = 0;
    if (mousepos.Y > frm_height - 1) mousepos.Y = frm_height - 1;

    clearcursor(cursorinside, mouseposold);

    static bool circle = false;
    switch(get_mousestate())
    {
        case ps2_left:
            if (circle)
            {
                drawfilledcircle(mousepos.X, mousepos.Y, 5, 0xff0000);   
            }
            else
            {
                drawfilledrectangle(mousepos.X, mousepos.Y, 10, 10, 0xff0000);
            }
            break;
        case ps2_middle:
            if (circle) circle = false;
            else circle = true;
            break;
        case ps2_right:
            if (circle)
            {
                drawfilledcircle(mousepos.X, mousepos.Y, 5, 0xdd56f5);   
            }
            else
            {
                drawfilledrectangle(mousepos.X, mousepos.Y, 10, 10, 0xdd56f5);
            }
            break;
    }

    drawovercursor(cursorinside, mousepos, 0x2d2d2d, true);
    drawovercursor(cursorborder, mousepos, 0xffffff, false);
    
    packetready = false;
    mouseposold = mousepos;
}

static void Mouse_Handler(interrupt_registers *)
{
    uint8_t mousedata = inb(0x60);

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
            if (packetready) break;
            if ((mousedata & 0b00001000) == 0) break;
            packet[0] = mousedata;
            cycle++;
            break;
        case 1:
            if (packetready) break;
            packet[1] = mousedata;
            cycle++;
            break;
        case 2:
            if (packetready) break;
            packet[2] = mousedata;
            packetready = true;
            cycle = 0;
            break;
    }
}

void Mouse_init()
{
    serial_info("Initializing PS2 mouse");

    register_interrupt_handler(IRQ12, Mouse_Handler);

    outb(0x64, 0xA8);

    mousewait();
    outb(0x64, 0x20);
    mousewait_input();

    uint8_t status = inb(0x60);
    status |= 0b10;
    mousewait();

    outb(0x64, 0x60);
    mousewait();
    outb(0x60, status);

    mousewrite(0xF6);
    mouseread();
    mousewrite(0xF4);
    mouseread();
    
    serial_info("Initialized PS2 mouse\n");
}