#include <stdint.h>
#include <system/idt/idt.hpp>
#include <include/string.hpp>
#include <include/io.hpp>
#include <drivers/serial/serial.hpp>
#include <drivers/terminal/terminal.hpp>
#include <drivers/keyboard/keyboard.hpp>
#include <drivers/keyboard/kbscancodetable.hpp>

static kbd_mod_t kbd_mod;

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
void handle_comb(char c)
{
    // Example: ctrl + alt + w
    if (kbd_mod.ctrl && kbd_mod.alt && (c == 'w') || (c == 'W')) printf("It works! ");
}

// Keyboard buffer
char* buff;
char c[10] = "\0";

// Clear keyboard buffer
void clearbuff()
{
    for (int i = 0; i < strlen(buff); i++)
    {
        buff[i] = '\0';
    }
}

volatile bool pressed = false;
volatile bool enter = false;

// Main keyboard handler
static void Keyboard_Handler(struct interrupt_registers *)
{
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80)
    {
        switch (scancode)
        {
            case L_SHIFT_UP:
            case R_SHIFT_UP:
                kbd_mod.shift = 0;
                break;
            case CTRL_UP:
                kbd_mod.ctrl = 0;
                break;
            case ALT_UP:
                kbd_mod.alt = 0;
                break;
        }
    }
    else
    {
        switch (scancode)
        {
            case L_SHIFT_DOWN:
            case R_SHIFT_DOWN:
                kbd_mod.shift = 1;
                break;
            case CTRL_DOWN:
                kbd_mod.ctrl = 1;
                break;
            case ALT_DOWN:
                kbd_mod.alt = 1;
                break;
            case CAPSLOCK:
                kbd_mod.capslock = (!kbd_mod.capslock) ? 1 : 0;
                break;
            case NUMLOCK:
                kbd_mod.numlock = (!kbd_mod.numlock) ? 1 : 0;
                break;
            case SCROLLLOCK:
                kbd_mod.scrolllock = (!kbd_mod.scrolllock) ? 1 : 0;
                break;
            case UP:
                strcpy(c, "\033[A");
                term_print(c);
                break;
            case DOWN:
                strcpy(c, "\033[B");
                term_print(c);
                break;
            case RIGHT:
                strcpy(c, "\033[C");
                term_print(c);
                break;
            case LEFT:
                strcpy(c, "\033[D");
                term_print(c);
                break;
            default:
                memset(c, 0, strlen(c));
                c[0] = get_ascii_char(scancode);
                if (kbd_mod.alt || kbd_mod.ctrl)
                {
                    handle_comb(c[0]);
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

char* getline()
{
    static char retstr[1024] = "\0";
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
    return retstr;
}

void Keyboard_init()
{
    serial_info("Initializing keyboard");

    register_interrupt_handler(IRQ1, Keyboard_Handler);
    buff[0] = '\0';
    
    serial_info("Initialized keyboard\n");
}
