// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <lib/lock.hpp>

using namespace kernel::drivers::display;

DEFINE_LOCK(log_lock)

void log(const char *fmt, ...)
{
    acquire_lock(log_lock);
    va_list args;
    va_start(args, fmt);
    vfctprintf(serial::printc, nullptr, "[INFO] ", args);
    vfctprintf(serial::printc, nullptr, fmt, args);
    vfctprintf(serial::printc, nullptr, "\n", args);
    va_end(args);
    release_lock(log_lock);
}

void warn(const char *fmt, ...)
{
    acquire_lock(log_lock);
    va_list args;
    va_start(args, fmt);
    vfctprintf(serial::printc, nullptr, "[\033[33mWARN\033[0m] ", args);
    vfctprintf(serial::printc, nullptr, fmt, args);
    vfctprintf(serial::printc, nullptr, "\n", args);
    va_end(args);
    release_lock(log_lock);
}

void error(const char *fmt, ...)
{
    acquire_lock(log_lock);
    va_list args;
    va_start(args, fmt);
    vfctprintf(serial::printc, nullptr, "[\033[31mERROR\033[0m] ", args);
    vfctprintf(serial::printc, nullptr, fmt, args);
    vfctprintf(serial::printc, nullptr, "\n", args);
    va_end(args);
    release_lock(log_lock);
}