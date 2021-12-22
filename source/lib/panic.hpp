// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/log.hpp>
#include <stddef.h>

#define ASSERT(x, msg) ({ \
    if (!(x)) \
    { \
        error("%s", (msg)); \
        return; \
    } \
})

[[noreturn]] void panic(const char *message, const char *file, size_t line);

#define PANIC(b) (panic(b, __FILE__, __LINE__))