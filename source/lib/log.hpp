// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <drivers/display/serial/serial.hpp>

using namespace kernel::drivers::display;

int log(const char *fmt, ...);
int warn(const char *fmt, ...);
int error(const char *fmt, ...);