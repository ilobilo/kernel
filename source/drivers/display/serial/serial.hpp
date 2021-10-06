#pragma once

#define COM1 0x3F8

void serial_printc(char c, void *arg);

void serial_printf(const char *fmt, ...);

void serial_info(const char *fmt, ...);

void serial_err(const char *fmt, ...);

void serial_newline();

void serial_init();
