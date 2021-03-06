// Copyright (C) 2021-2022  ilobilo

#include <drivers/ps2/kbscancodetable/kbscancodetable.hpp>
#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/fs/devfs/dev/tty.hpp>
#include <drivers/vmware/vmware.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/acpi/acpi.hpp>
#include <drivers/ps2/ps2.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs::dev;
using namespace kernel::system::cpu;
using namespace kernel::system;

namespace kernel::drivers::ps2 {

bool initialised = false;

uint8_t kbdtype = 0;
uint8_t mousetype = 0;

devices kbddevice = PS2_DEVICE_NONE;
devices mousedevice = PS2_DEVICE_NONE;

#pragma region ps2utils

static bool canread()
{
    return (inb(PS2_PORT_STATUS) & PS2_OUTPUT_FULL) != 0;
}

static bool canwrite()
{
    return (inb(PS2_PORT_STATUS) & PS2_INPUT_FULL) == 0;
}

static uint8_t read()
{
    for (size_t i = 0; i < PS2_MAX_TIMEOUT; i++)
    {
        if (canread()) return inb(PS2_PORT_DATA);
    }
    return 0;
}

static uint8_t read(bool &error)
{
    for (size_t i = 0; i < PS2_MAX_TIMEOUT; i++)
    {
        if (canread())
        {
            error = false;
            return inb(PS2_PORT_DATA);
        }
    }
    error = true;
    return 0;
}

static void write(uint16_t port, uint8_t value)
{
    for (size_t i = 0; i < PS2_MAX_TIMEOUT; i++)
    {
        if (canwrite()) return outb(port, value);
    }
}

static void drainbuffer()
{
    while (!canwrite()) inb(PS2_PORT_DATA);
}

static void disabledev(devices device)
{
    switch (device)
    {
        case PS2_DEVICE_FIRST:
            write(PS2_PORT_COMMAND, PS2_DISABLE_FIRST);
            break;
        case PS2_DEVICE_SECOND:
            write(PS2_PORT_COMMAND, PS2_DISABLE_SECOND);
            break;
        default:
            error("PS/2: Unknown device!");
            break;
    }
}

static void enabledev(devices device)
{
    switch (device)
    {
        case PS2_DEVICE_FIRST:
            write(PS2_PORT_COMMAND, PS2_ENABLE_FIRST);
            break;
        case PS2_DEVICE_SECOND:
            write(PS2_PORT_COMMAND, PS2_ENABLE_SECOND);
            break;
        default:
            error("PS/2: Unknown device!");
            break;
    }
}

static uint8_t readconfig()
{
    write(PS2_PORT_COMMAND, PS2_READ_CONFIG);
    return read();
}

static void writeconfig(uint8_t config)
{
    write(PS2_PORT_COMMAND, PS2_WRITE_CONFIG);
    write(PS2_PORT_DATA, config);
}

static bool writedev(devices device, uint8_t value)
{
    for (size_t i = 0; i < PS2_MAX_RESENDS; i++)
    {
        if (device == PS2_DEVICE_SECOND) write(PS2_PORT_COMMAND, PS2_WRITE_SECOND);
        write(PS2_PORT_DATA, value);

        while (true)
        {
            uint8_t ret = read();
            if (ret == 0xFA)
            {
                if (value == PS2_DEVICE_RESET)
                {
                    if (read() == 0xAA) return true;
                    return false;
                }
                return true;
            }
            if (ret == 0xFE) break;
            if (ret == 0xFF) return false;
        }
    }
    return false;
}

// static bool checkint(devices device)
// {
//     disabledev(PS2_DEVICE_FIRST);
//     disabledev(PS2_DEVICE_SECOND);

//     uint8_t status = inb(PS2_PORT_STATUS);
//     // uint8_t data = inb(PS2_PORT_DATA);

//     enabledev(PS2_DEVICE_FIRST);
//     enabledev(PS2_DEVICE_SECOND);

//     if (!(status & 0x01)) return false;

//     if (!(status & 0x20) && device == PS2_DEVICE_FIRST) return true;
//     else if ((status & 0x21) && device == PS2_DEVICE_SECOND) return true;
//     return false;
// }

#pragma endregion ps2utils

#pragma region kbd

bool kbdinitialised = false;

new_lock(kbd_lock);

void setscancodeset(uint8_t scancode)
{
    writedev(kbddevice, PS2_KEYBOARD_SCANCODE_SET);
    writedev(kbddevice, scancode);
}

uint8_t getscancodeset()
{
    writedev(kbddevice, PS2_KEYBOARD_SCANCODE_SET);
    writedev(kbddevice, 0);
    return read() & 3;
}

void update_leds()
{
    writedev(kbddevice, PS2_KEYBOARD_SET_LEDS);
    writedev(kbddevice, tty::current_tty->leds);
}

extern void (*handlers[])(bool, uint8_t);

static void latin_handler(bool up, uint8_t value)
{
    if (up == true) return;
    tty::current_tty->add_char(static_cast<char>(value));
}
static void func_handler(bool up, uint8_t value)
{
    if (up == true) return;
    tty::current_tty->add_str(func_table[value]);
}
static void spec_handler(bool up, uint8_t value)
{
    if (up == true) return;
    switch (value)
    {
        case 1:
            tty::current_tty->add_char('\n');
            break;
        case 7:
            tty::current_tty->capslock = !tty::current_tty->capslock;
            break;
        case 8:
            tty::current_tty->numlock = !tty::current_tty->numlock;
            break;
        case 9:
            tty::current_tty->scrollock = !tty::current_tty->scrollock;
            break;
        case 12:
            acpi::reboot();
            break;
        case 13:
            tty::current_tty->capslock = true;
            break;
        case 19:
            tty::current_tty->numlock = !tty::current_tty->numlock;
    }
}
static void pad_handler(bool up, uint8_t value)
{
    static const char pad_chars[] = "0123456789+-*/\015,.?()#";

    if (up == true) return;

    if (!tty::current_tty->numlock)
    {
        switch (value)
        {
            case KEY_VALUE(KEY_PCOMMA):
            case KEY_VALUE(KEY_PDOT):
                handlers[1](up, KEY_VALUE(KEY_REMOVE));
                return;
            case KEY_VALUE(KEY_P0):
                handlers[1](up, KEY_VALUE(KEY_INSERT));
                return;
            case KEY_VALUE(KEY_P1):
                handlers[1](up, KEY_VALUE(KEY_SELECT));
                return;
            case KEY_VALUE(KEY_P2):
                handlers[6](up, KEY_VALUE(KEY_DOWN));
                return;
            case KEY_VALUE(KEY_P3):
                handlers[1](up, KEY_VALUE(KEY_PGDN));
                return;
            case KEY_VALUE(KEY_P4):
                handlers[6](up, KEY_VALUE(KEY_LEFT));
                return;
            case KEY_VALUE(KEY_P5):
                tty::current_tty->add_str("\033[G");
                return;
            case KEY_VALUE(KEY_P6):
                handlers[6](up, KEY_VALUE(KEY_RIGHT));
                return;
            case KEY_VALUE(KEY_P7):
                handlers[1](up, KEY_VALUE(KEY_FIND));
                return;
            case KEY_VALUE(KEY_P8):
                handlers[6](up, KEY_VALUE(KEY_UP));
                return;
            case KEY_VALUE(KEY_P9):
                handlers[1](up, KEY_VALUE(KEY_PGUP));
                return;
        }
    }
    tty::current_tty->add_char(pad_chars[value]);
}
static void dead_handler(bool up, uint8_t value) { }
static void cons_handler(bool up, uint8_t value) { }
static void cur_handler(bool up, uint8_t value)
{
    if (up == true) return;
    static const char cur_chars[] = "BDCA";
    tty::current_tty->add_str((char[]) { '\033', (tty::current_tty->decckm ? 'O' : '['), cur_chars[value], '\0' });
}
static void shift_handler(bool up, uint8_t value)
{
    if (up == true) tty::current_tty->shiftstate &= ~(1 << value);
    else tty::current_tty->shiftstate |= (1 << value);
}
static void meta_handler(bool up, uint8_t value)
{
    if (up == true) return;
    tty::current_tty->add_char('\033');
    tty::current_tty->add_char(value);
}
static void ascii_handler(bool up, uint8_t value) { }
static void lock_handler(bool up, uint8_t value)
{
    if (up == true) return;
    tty::current_tty->lockstate ^= 1 << value;
}
static void letter_handler(bool up, uint8_t value) { }
static void slock_handler(bool up, uint8_t value)
{
    handlers[7](up, value);
    if (up == true) return;

    tty::current_tty->slockstate ^= 1 << value;
    if (!key_maps[tty::current_tty->lockstate ^ tty::current_tty->slockstate])
    {
        tty::current_tty->slockstate = 0;
        tty::current_tty->slockstate ^= 1 << value;
    }
}
static void dead2_handler(bool up, uint8_t value) { }
static void brl_handler(bool up, uint8_t value) { }

void (*handlers[15])(bool, uint8_t)
{
    latin_handler, func_handler, spec_handler,
    pad_handler, dead_handler, cons_handler,
    cur_handler, shift_handler, meta_handler,
    ascii_handler, lock_handler, letter_handler,
    slock_handler, dead2_handler, brl_handler
};

static void Keyboard_handler(registers_t *)
{
    // if (checkint(kbddevice) == false) return;
    lockit(kbd_lock);
    uint8_t scancode = inb(PS2_PORT_DATA);
    if (scancode == 0xE0) return;
    if (scancode > 0xFF) return;

    static uint8_t last_leds = 0;
    bool up = scancode & 0x80;
    scancode &= ~0x80;

    int map_i = (tty::current_tty->shiftstate | tty::current_tty->slockstate) ^ tty::current_tty->lockstate;
    uint16_t *map = key_maps[map_i];
    if (map == nullptr) map = plain_map;

    uint16_t keysym = map[scancode];
    uint8_t type = KEY_TYPE(keysym);

    if (type >= 0xF0)
    {
        type -= 0xF0;
        if (type == KEY_TYPE_LETTER)
        {
            type = KEY_TYPE_LATIN;
            if (tty::current_tty->capslock)
            {
                map = key_maps[map_i ^ (1 << KG_SHIFT)];
                if (map) keysym = map[scancode];
            }
        }
        handlers[type](up, KEY_VALUE(keysym));
    }
    else error("Error: Type < 0xF0?");

    if (last_leds != tty::current_tty->leds) update_leds();
    last_leds = tty::current_tty->leds;
}

static bool setupKbd(devices device)
{
    if (kbdinitialised) return false;
    log("PS/2: Setting up keyboard");

    kbddevice = device;

    writedev(kbddevice, PS2_DEVICE_ENABLE);

    setscancodeset(PS2_KBD_SCANCODE);
    log("PS/2: Current scancode set is: %d", getscancodeset());

    update_leds();
    idt::register_interrupt_handler(idt::IRQ1, Keyboard_handler, true);

    kbdinitialised = true;
    return kbdinitialised;
}

#pragma endregion kbd

#pragma region mouse

bool mouseinitialised = false;
bool mousevmware = false;

static uint8_t mousecycle = 0;
static uint8_t mousepacket[4];
static bool packetready = false;
point mousepos;
point mouseposold;

uint32_t mousebordercol = 0xFFFFFF;
uint32_t mouseinsidecol = 0x2D2D2D;

static uint8_t cursorborder[]
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

static uint8_t cursorinside[]
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

mousestate getmousestate()
{
    if (mousevmware) return vmware::getmousestate();
    if (mousepacket[0] & PS2_LEFT) return PS2_LEFT;
    else if (mousepacket[0] & PS2_MIDDLE) return PS2_MIDDLE;
    else if (mousepacket[0] & PS2_RIGHT) return PS2_RIGHT;
    return PS2_NONE;
}

void mousedraw()
{
    framebuffer::drawovercursor(cursorinside, mousepos, mouseinsidecol, true);
    framebuffer::drawovercursor(cursorborder, mousepos, mousebordercol, false);
}

void mouseclear()
{
    framebuffer::clearcursor(cursorinside, mouseposold);
}

void proccesspacket()
{
    if (!packetready) return;

    bool xmin, ymin, xover, yover;

    if (mousepacket[0] & PS2_X_SIGN) xmin = true;
    else xmin = false;

    if (mousepacket[0] & PS2_Y_SIGN) ymin = true;
    else ymin = false;

    if (mousepacket[0] & PS2_X_OVER) xover = true;
    else xover = false;

    if (mousepacket[0] & PS2_Y_OVER) yover = true;
    else yover = false;

    if (!xmin)
    {
        mousepos.X += mousepacket[1];
        if (xover) mousepos.X += 255;
    }
    else
    {
        mousepacket[1] = 256 - mousepacket[1];
        mousepos.X -= mousepacket[1];
        if (xover) mousepos.X -= 255;
    }

    if (!ymin)
    {
        mousepos.Y -= mousepacket[2];
        if (yover) mousepos.Y -= 255;
    }
    else
    {
        mousepacket[2] = 256 - mousepacket[2];
        mousepos.Y += mousepacket[2];
        if (yover) mousepos.Y += 255;
    }

    if (mousepos.X < 0) mousepos.X = 0;
    if (mousepos.X > framebuffer::main_frm->width - 1) mousepos.X = framebuffer::main_frm->width - 1;

    if (mousepos.Y < 0) mousepos.Y = 0;
    if (mousepos.Y > framebuffer::main_frm->height - 1) mousepos.Y = framebuffer::main_frm->height - 1;

    mouseclear();

    static bool circle = false;
    switch(getmousestate())
    {
        case PS2_LEFT:
            if (circle) framebuffer::drawfilledcircle(mousepos.X, mousepos.Y, 5, 0xFF0000);
            else framebuffer::drawfilledrectangle(mousepos.X, mousepos.Y, 10, 10, 0xFF0000);
            break;
        case PS2_MIDDLE:
            if (circle) circle = false;
            else circle = true;
            break;
        case PS2_RIGHT:
            if (circle) framebuffer::drawfilledcircle(mousepos.X, mousepos.Y, 5, 0xDD56F5);
            else framebuffer::drawfilledrectangle(mousepos.X, mousepos.Y, 10, 10, 0xDD56F5);
            break;
        default:
            break;
    }

    mousedraw();

    packetready = false;
    mouseposold = mousepos;
}

static void Mouse_Handler(registers_t *)
{
    // if (checkint(mousedevice) == false) return;
    uint8_t mousedata = inb(0x60);

    if (mousevmware)
    {
        vmware::handle_mouse();
        return;
    }

    proccesspacket();

    static bool skip = true;
    if (skip)
    {
        skip = false;
        return;
    }

    switch (mousecycle)
    {
        case 0:
            if ((mousedata & 0b00001000) == 0) break;
            mousepacket[0] = mousedata;
            mousecycle++;
            break;
        case 1:
            mousepacket[1] = mousedata;
            mousecycle++;
            break;
        case 2:
            mousepacket[2] = mousedata;
            packetready = true;
            mousecycle = 0;
            break;
    }
}

static bool setupMouse(devices device)
{
    if (mouseinitialised) return false;
    log("PS/2: Setting up mouse");

    mousedevice = device;

    writedev(mousedevice, PS2_MOUSE_DEFAULTS);
    writedev(mousedevice, PS2_DEVICE_ENABLE);

    idt::register_interrupt_handler(idt::IRQ12, Mouse_Handler, true);

    mouseinitialised = true;
    return mouseinitialised;
}

#pragma endregion mouse

static bool initdev(devices device)
{
    if (device == PS2_DEVICE_SECOND)
    {
        if (!writedev(device, PS2_DEVICE_RESET))
        {
            error("PS/2: Device reset failed!");
            return false;
        }
    }

    writedev(device, PS2_DEVICE_DISABLE);
    writedev(device, PS2_DEVICE_IDENTIFY);

    bool kbd = false;
    uint8_t devtype = read(kbd);
    if (kbd) return setupKbd(device);

    switch (devtype)
    {
        case 0x00:
        case 0x03:
        case 0x04:
            return setupMouse(device);
            break;
        case 0xAB:
            devtype = read();
            switch (devtype)
            {
                case 0x41:
                case 0xC1:
                case 0x83:
                    return setupKbd(device);
                default:
                    error("PS/2: Unknown keyboard type 0x%X!", devtype);
                    break;
            }
            break;
        default:
            error("PS/2: Unknown device type 0x%X!", devtype);
            break;
    }
    return false;
}

void reboot()
{
    if (!initialised) return;
    uint8_t good = 0x02;
    while (good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE);
}

void init()
{
    log("Initialising PS/2 controller");

    if (initialised)
    {
        warn("PS/2 controller has already been initialised!\n");
        return;
    }

    drainbuffer();

    disabledev(PS2_DEVICE_FIRST);
    disabledev(PS2_DEVICE_SECOND);

    drainbuffer();

    uint8_t config = readconfig() & ~(PS2_FIRST_IRQ_MASK | PS2_SECOND_IRQ_MASK | PS2_TRANSLATION);
    writeconfig(config);

    write(PS2_PORT_COMMAND, PS2_TEST_CONTROLER);
    if (read() != 0x55)
    {
        error("PS/2: Controller self-test failed!");
        return;
    }

    bool dualchannel = false;
    if (config & PS2_SECOND_CLOCK)
    {
        enabledev(PS2_DEVICE_SECOND);
        config = readconfig();

        if (!(config & PS2_SECOND_CLOCK)) dualchannel = true;
        else error("PS/2: Controller is not dual channel!");
    }

    write(PS2_PORT_COMMAND, PS2_TEST_FIRST);
    if (read() != 0x00)
    {
        error("PS/2: Primary port self-test failed!");
        return;
    }
    else
    {
        enabledev(PS2_DEVICE_FIRST);
        if (!initdev(PS2_DEVICE_FIRST)) error("PS/2: Could not initialise primary port!");
    }

    if (dualchannel)
    {
        write(PS2_PORT_COMMAND, PS2_TEST_SECOND);
        if (read() != 0x00)
        {
            error("PS/2: Secondary port self-test has failed!");
            return;
        }
        else
        {
            enabledev(PS2_DEVICE_SECOND);
            if (!initdev(PS2_DEVICE_SECOND)) error("PS/2: Could not initialise secondary port");
        }
    }

    config = readconfig() | PS2_FIRST_IRQ_MASK | PS2_SECOND_IRQ_MASK | PS2_TRANSLATION;
    writeconfig(config);

    serial::newline();
    initialised = true;
}
}