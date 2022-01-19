// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/log.hpp>
#include <stddef.h>

[[noreturn]] void panic(const char *message, const char *file, size_t line);

#define PANIC(b) (panic(b, __FILE__, __LINE__))

#define ASSERT(x, msg) (!(x) ? PANIC(msg) : static_cast<void>(const_cast<char*>(msg)))