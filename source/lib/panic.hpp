// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/log.hpp>
#include <cstddef>
#include <utility>

// [[noreturn]] void panic(const char *message, std::source_location location = std::source_location::current());

[[noreturn]] void panic(const char *message, std::source_location location);
#define panic(msg) panic((msg), std::source_location::current())

#define ASSERT_MSG(x, msg) (!(x) ? panic(msg) : static_cast<void>(const_cast<char*>(msg)))
#define ASSERT_NOMSG(x) (!(x) ? panic("Assertion failed: " #x) : static_cast<void>(x))
#define GET_MACRO(_1, _2, NAME, ...) NAME

#define assert(...) GET_MACRO(__VA_ARGS__, ASSERT_MSG, ASSERT_NOMSG)(__VA_ARGS__)