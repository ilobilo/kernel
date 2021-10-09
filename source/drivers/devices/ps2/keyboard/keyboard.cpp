#include <drivers/devices/ps2/keyboard/kbscancodetable.hpp>
#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/idt/idt.hpp>
#include <lib/string.hpp>
#include <lib/io.hpp>

bool kbd_initialised = false;

char retstr[1024] = "\0";
bool reading = false;

volatile bool pressed = false;
volatile bool enter = false;

kbd_mod_t kbd_mod;

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
}

// Handle key combinations
void handle_comb(uint8_t scancode)
{
    char ch = get_ascii_char(scancode);

    // Crash the os: CTRL + ALT + DEL
    if (kbd_mod.ctrl && kbd_mod.alt && scancode == keys::DELETE)
    {
        asm volatile ("int $0x3");
        asm volatile ("int $0x4");
    }
    else if (kbd_mod.ctrl && (ch == 'l') || (ch == 'L'))
    {
        term_clear();
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
    for (int i = 0; i < strlen(buff); i++)
    {
        buff[i] = '\0';
    }
}

// Main keyboard handler
static void Keyboard_Handler(interrupt_registers *)
{
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80)
    {
        switch (scancode)
        {
            case keys::L_SHIFT_UP:
            case keys::R_SHIFT_UP:
                kbd_mod.shift = 0;
                break;
            case keys::CTRL_UP:
                kbd_mod.ctrl = 0;
                break;
            case keys::ALT_UP:
                kbd_mod.alt = 0;
                break;
        }
    }
    else
    {
        switch (scancode)
        {
            case keys::L_SHIFT_DOWN:
            case keys::R_SHIFT_DOWN:
                kbd_mod.shift = 1;
                break;
            case keys::CTRL_DOWN:
                kbd_mod.ctrl = 1;
                break;
            case keys::ALT_DOWN:
                kbd_mod.alt = 1;
                break;
            case keys::CAPSLOCK:
                kbd_mod.capslock = (!kbd_mod.capslock) ? 1 : 0;
                break;
            case keys::NUMLOCK:
                kbd_mod.numlock = (!kbd_mod.numlock) ? 1 : 0;
                break;
            case keys::SCROLLLOCK:
                kbd_mod.scrolllock = (!kbd_mod.scrolllock) ? 1 : 0;
                break;
            case keys::UP:
                strcpy(c, "\033[A");
                term_print(c);
                break;
            case keys::DOWN:
                strcpy(c, "\033[B");
                term_print(c);
                break;
            case keys::RIGHT:
                strcpy(c, "\033[C");
                term_print(c);
                break;
            case keys::LEFT:
                strcpy(c, "\033[D");
                term_print(c);
                break;
            default:
                memset(c, 0, strlen(c));
                c[0] = get_ascii_char(scancode);
                if (kbd_mod.alt || kbd_mod.ctrl)
                {
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
                                printf("\b \b");
                            }
                            break;
                        default:
                            pressed = true;
                            term_print(c);
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

char *getline()
{
    reading = true;
    memset(retstr, '\0', 1024);
    int i = 0;
    while (!enter)
    {
        if (pressed)
        {
            if (i >= 1024 - 1)
            {
                printf("\nBuffer Overflow");
            }
            retstr[i] = getchar();
            i++;
        }
    }
    enter = false;
    reading = false;
    return retstr;
}

void Keyboard_init()
{
    serial_info("Initialising PS2 keyboard\n");

    if (kbd_initialised)
    {
        serial_info("Keyboard driver has already been initialised!\n");
        return;
    }

    if (!idt_initialised)
    {
        serial_info("IDT has not been initialised!\n");
        IDT_init();
    }

    register_interrupt_handler(IRQ1, Keyboard_Handler);
    buff[0] = '\0';

    kbd_initialised = true;
}
