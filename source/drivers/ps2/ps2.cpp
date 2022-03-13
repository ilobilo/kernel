// Copyright (C) 2021-2022  ilobilo

#include <drivers/ps2/kbscancodetable/kbscancodetable.hpp>
#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
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

#pragma endregion ps2utils

#pragma region kbd

bool kbdinitialised = false;

char *buff = nullptr;
char c[10] = "\0";

char *retstr = nullptr;
bool reading = false;
size_t gi = 0;

volatile bool pressed = false;
volatile bool enter = false;

kbd_mod_t kbd_mod;
DEFINE_LOCK(kbd_lock)
DEFINE_LOCK(getline_lock)

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

static char get_ascii_char(uint8_t key_code)
{
    if (!kbd_mod.shift && !kbd_mod.capslock)
    {
        return kbdus[key_code];
    }
    if (kbd_mod.shift && !kbd_mod.capslock)
    {
        return kbdus_shft[key_code];
    }
    if (!kbd_mod.shift && kbd_mod.capslock)
    {
        return kbdus_caps[key_code];
    }
    if (kbd_mod.shift && kbd_mod.capslock)
    {
        return kbdus_capsshft[key_code];
    }
    return 0;
}

void clearbuff()
{
    if (!kbdinitialised) return;
    memset(buff, 0, KBD_BUFFSIZE);
}

void clearretstr()
{
    if (!kbdinitialised) return;
    memset(retstr, 0, KBD_BUFFSIZE);
}

static void handle_comb(uint8_t scancode)
{
    char ch = get_ascii_char(scancode);

    // Reboot the os: CTRL + ALT + DEL
    if (kbd_mod.ctrl && kbd_mod.alt && scancode == keys::DELETE) acpi::reboot();
    else if (kbd_mod.ctrl && ((ch == 'r') || (ch == 'R'))) terminal::reset();
    else if (kbd_mod.ctrl && ((ch == 'l') || (ch == 'L')))
    {
        terminal::clear();
        terminal::reset();
        if (reading)
        {
            clearretstr();
            enter = true;
        }
        clearbuff();
    }
}

static void update_leds()
{
    uint8_t value = 0b000;
    if (kbd_mod.scrolllock) value |= (1 << 0);
    if (kbd_mod.numlock) value |= (1 << 1);
    if (kbd_mod.capslock) value |= (1 << 2);
    writedev(kbddevice, PS2_KEYBOARD_SET_LEDS);
    writedev(kbddevice, value);
}

char getchar()
{
    if (!kbdinitialised) return 0;
    while (!pressed);
    pressed = false;
    return c[0];
}

char *getline()
{
    if (!kbdinitialised) return nullptr;
    getline_lock.lock();
    reading = true;
    memset(retstr, '\0', KBD_BUFFSIZE);
    while (!enter)
    {
        if (pressed)
        {
            if (gi >= KBD_BUFFSIZE - 1)
            {
                printf("\nBuffer Overflow\n");
                enter = false;
                reading = false;
                gi = 0;
                getline_lock.unlock();
                return nullptr;
            }
            retstr[gi] = getchar();
            gi++;
        }
    }
    enter = false;
    reading = false;
    gi = 0;
    getline_lock.unlock();
    return retstr;
}

static void Keyboard_handler(registers_t *)
{
    uint8_t scancode = inb(PS2_PORT_DATA);
    kbd_lock.lock();

    if (scancode & 0x80)
    {
        switch (scancode)
        {
            case keys::L_SHIFT_UP:
            case keys::R_SHIFT_UP:
                kbd_mod.shift = false;
                break;
            case keys::CTRL_UP:
                kbd_mod.ctrl = false;
                break;
            case keys::ALT_UP:
                kbd_mod.alt = false;
                break;
        }
    }
    else
    {
        switch (scancode)
        {
            case keys::L_SHIFT_DOWN:
            case keys::R_SHIFT_DOWN:
                kbd_mod.shift = true;
                break;
            case keys::CTRL_DOWN:
                kbd_mod.ctrl = true;
                break;
            case keys::ALT_DOWN:
                kbd_mod.alt = true;
                break;
            case keys::CAPSLOCK:
                kbd_mod.capslock = (!kbd_mod.capslock) ? true : false;
                update_leds();
                break;
            case keys::NUMLOCK:
                kbd_mod.numlock = (!kbd_mod.numlock) ? true : false;
                update_leds();
                break;
            case keys::SCROLLLOCK:
                kbd_mod.scrolllock = (!kbd_mod.scrolllock) ? true : false;
                update_leds();
                break;
            default:
                memset(c, 0, strlen(c));
                c[0] = get_ascii_char(scancode);
                if (kbd_mod.alt || kbd_mod.ctrl)
                {
                    // char ch = toupper(c[0]);

                    // if (kbd_mod.ctrl)
                    // {
                    //     if ((ch >= 'A' && ch <= '_') || ch == '?' || ch == '0')
                    //     {
                    //         printf("%c", escapes[tonum(ch)]);
                    //     }
                    // }
                    // else if (kbd_mod.alt) printf("\x1b[%c", ch);
                    handle_comb(scancode);
                }
                else
                {
                    switch (c[0])
                    {
                        case '\n':
                            printf("\n");
                            clearbuff();
                            enter = true;
                            break;
                        case '\b':
                            if (buff[0] != '\0')
                            {
                                buff[strlen(buff) - 1] = '\0';
                                if (reading) retstr[--gi] = 0;
                                printf("\b \b");
                            }
                            break;
                        default:
                            pressed = true;
                            printf("%s", c);
                            strcat(buff, c);
                            break;
                    }
                }
                break;
        }
    }
    kbd_lock.unlock();
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
    buff = new char[KBD_BUFFSIZE];
    retstr = new char[KBD_BUFFSIZE];

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
    if (mousepos.X > framebuffer::frm_width - 1) mousepos.X = framebuffer::frm_width - 1;

    if (mousepos.Y < 0) mousepos.Y = 0;
    if (mousepos.Y > framebuffer::frm_height - 1) mousepos.Y = framebuffer::frm_height - 1;

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
        default:
            error("PS/2: Unknown device type 0x%X!", devtype);
            break;
    }
    return false;
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