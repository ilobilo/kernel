// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/display/serial/serial.hpp>

using namespace kernel::drivers::display;

void log(const char *fmt, ...);
void warn(const char *fmt, ...);
void error(const char *fmt, ...);