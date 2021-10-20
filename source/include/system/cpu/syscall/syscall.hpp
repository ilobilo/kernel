#pragma once

#include <system/cpu/idt/idt.hpp>

char *syscall_read(int length);
void syscall_write(char *string);
void syscall_err(char *string);

extern "C" void syscall_handler(interrupt_registers *regs);