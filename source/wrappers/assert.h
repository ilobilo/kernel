// Copyright (C) 2021-2022  ilobilo

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((noreturn)) void panic(const char *message, const char *file, const char *func, int line);

#ifdef __cplusplus
}
#endif

#define assert(x) (!(x) ? panic("Assertion failed: " #x, __FILE__, __PRETTY_FUNCTION__, __LINE__) : (void)(x))