#pragma once

#define COM1 0x3F8

void serial_printc(char c);

void serial_printstr(char* str);

void serial_info(char* str);

void serial_err(char* str);

void serial_init();
