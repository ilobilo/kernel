#pragma once

#include <drivers/display/serial/serial.hpp>
#include <stddef.h>

#define ASSERT(x, msg) ({ \
    if (!(x)) \
    { \
        kernel::drivers::display::serial::err("%s", (msg)); \
        return; \
    } \
})

[[noreturn]] void panic(const char *message, const char *file, size_t line);

#define PANIC(b) (panic(b, __FILE__, __LINE__))