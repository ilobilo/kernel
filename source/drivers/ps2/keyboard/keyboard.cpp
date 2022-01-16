// Copyright (C) 2021  ilobilo

#include <drivers/ps2/keyboard/kbscancodetable.hpp>
#include <drivers/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/acpi/acpi.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::cpu;
using namespace kernel::system;

namespace kernel::drivers::ps2::kbd {

bool initialised = false;

char retstr[1024] = "\0";
bool reading = false;
int gi = 0;

volatile bool pressed = false;
volatile bool enter = false;

kbd_mod_t kbd_mod;

static bool wait_in()
{
    uint64_t timeout = 100000U;
    while (--timeout) if (!(inb(0x64) & (1 << 1))) return false;
    return true;
}

static bool wait_out()
{
	uint64_t timeout = 100000;
	while (--timeout) if (inb(0x64) & (1 << 0)) return false;
	return true;
}

static uint8_t kbd_write(uint8_t write)
{
	wait_in();
	outb(0x60, write);
	wait_out();
	return inb(0x60);
}

// Scancode to ascii
char get_ascii_char(uint8_t key_code)
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

// Handle key combinations
void handle_comb(uint8_t scancode)
{
    char ch = get_ascii_char(scancode);

    // Reboot the os: CTRL + ALT + DEL
    if (kbd_mod.ctrl && kbd_mod.alt && scancode == keys::DELETE) acpi::reboot();
    else if (kbd_mod.ctrl && ((ch == 'l') || (ch == 'L')))
    {
        terminal::clear();
        if (reading)
        {
            memset(retstr, '\0', 1024);
            enter = true;
        }
    }
}

// Keyboard buffer
char *buff;
char c[10] = "\0";

// Clear keyboard buffer
void clearbuff()
{
    for (size_t i = 0; i < strlen(buff); i++)
    {
        buff[i] = '\0';
    }
}

// Update keyboard LEDs
void update_leds()
{
    uint8_t value = 0b000;
    if (kbd_mod.scrolllock) value |= (1 << 0);
    if (kbd_mod.numlock) value |= (1 << 1);
    if (kbd_mod.capslock) value |= (1 << 2);
    kbd_write(0xED);
    kbd_write(value);
}

// Main keyboard handler
static void Keyboard_Handler(registers_t *)
{
    uint8_t scancode = inb(0x60);

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
            case keys::UP:
                strcpy(c, "\033[A");
                terminal::cursor_up();
                break;
            case keys::DOWN:
                strcpy(c, "\033[B");
                terminal::cursor_down();
                break;
            case keys::RIGHT:
                strcpy(c, "\033[C");
                terminal::cursor_right();
                break;
            case keys::LEFT:
                strcpy(c, "\033[D");
                terminal::cursor_left();
                break;
            default:
                memset(c, 0, strlen(c));
                c[0] = get_ascii_char(scancode);
                if (kbd_mod.alt || kbd_mod.ctrl)
                {
                    char ch = char2up(c[0]);

                    if (kbd_mod.ctrl)
                    {
                        if ((ch >= 'A' && ch <= '_') || ch == '?' || ch == '0')
                        {
                            printf("%c", escapes[char2num(ch)]);
                        }
                    }
                    else if (kbd_mod.alt) printf("\x1b[%c", ch);
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
}

char getchar()
{
    while (!pressed);
    pressed = false;
    return c[0];
}

DEFINE_LOCK(getline_lock)
char *getline()
{
    getline_lock.lock();
    reading = true;
    memset(retstr, '\0', 1024);
    while (!enter)
    {
        if (pressed)
        {
            if (gi >= 1024 - 1)
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

void init()
{
    log("Initialising PS/2 keyboard");

    if (initialised)
    {
        warn("PS/2 keyboard has already been initialised!\n");
        return;
    }

    // Set scancode table 2
    // kbd_write(0xF0);
    // kbd_write(2);

    update_leds();

    buff = static_cast<char*>(calloc(1024, sizeof(char)));
    idt::register_interrupt_handler(idt::IRQ1, Keyboard_Handler);

    serial::newline();
    initialised = true;
}
}