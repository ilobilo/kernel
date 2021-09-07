#include <drivers/serial/serial.hpp>
#include <include/io.hpp>

int is_transmit_empty(void)
{
	return inb(COM1 + 5) & 0x20;
}

void serial_printc(char c)
{
    while (is_transmit_empty() == 0);

    outb(COM1, c);
}

void serial_printstr(char* str)
{
    for (int i = 0; i < str[i] != '\0'; i++)
    {
        serial_printc(str[i]);
    }
}

void serial_info(char* str)
{
    serial_printstr("[\033[33mINFO\033[0m] ");
    serial_printstr(str);
    serial_printc('\n');
}

void serial_err(char* str)
{
    serial_printstr("[\033[31mERROR\033[0m] ");
    serial_printstr(str);
    serial_printc('\n');
}

void serial_init()
{
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);

    serial_printstr("\033[H\033[2J");
//    serial_printc("\033[2J");
}
