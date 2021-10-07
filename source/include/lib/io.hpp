#pragma once

#include <stdint.h>
#include <stdbool.h>

void outb(uint16_t port, uint8_t val);

void outw(uint16_t port, uint16_t val);

void outl(uint16_t port, uint32_t val);

uint8_t inb(uint16_t port);

uint16_t inw(uint16_t port);

uint32_t inl(uint16_t port);

void io_wait(void);

bool are_interrupts_enabled(bool should_print);
