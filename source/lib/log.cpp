// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <lib/lock.hpp>

using namespace kernel::drivers::display;

new_lock(log_lock);

int log(const char *fmt, ...)
{
    lockit(log_lock);

    va_list args;
    va_start(args, fmt);
    int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "[\033[32mINFO\033[0m] ", args);
    ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, args);
    ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "\n", args);
    va_end(args);

    return ret;
}

int warn(const char *fmt, ...)
{
    lockit(log_lock);

    va_list args;
    va_start(args, fmt);
    int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "[\033[33mWARN\033[0m] ", args);
    ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, args);
    ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "\n", args);
    va_end(args);

    return ret;
}

int error(const char *fmt, ...)
{
    lockit(log_lock);

    va_list args;
    va_start(args, fmt);
    int ret = vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "[\033[31mERROR\033[0m] ", args);
    ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), fmt, args);
    ret += vfctprintf(serial::printc, reinterpret_cast<void*>(serial::COM1), "\n", args);
    va_end(args);

    return ret;
}