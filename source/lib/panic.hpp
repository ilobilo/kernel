// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/log.hpp>
#include <stddef.h>

[[noreturn]] void panic(const char *message, const char *file, const char *function, size_t line);

#define PANIC(b) (panic(b, __FILE__, __PRETTY_FUNCTION__, __LINE__))

#define ASSERT_NOMSG(x, msg) (!(x) ? PANIC(msg) : static_cast<void>(const_cast<char*>(msg)))
#define ASSERT_MSG(x) (!(x) ? PANIC("Assertion failed: " #x) : static_cast<void>(x))

#define GET_MACRO(_1,_2,NAME,...) NAME
#define ASSERT(...) GET_MACRO(__VA_ARGS__, ASSERT_NOMSG, ASSERT_MSG)(__VA_ARGS__)