// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <lib/lock.hpp>

using namespace kernel::drivers::display;

DEFINE_LOCK(log_lock)

int log(const char *fmt, ...)
{
    log_lock.lock();
    va_list args;
    va_start(args, fmt);
    int ret = vfctprintf(serial::printc, nullptr, "[INFO] ", args);
    ret += vfctprintf(serial::printc, nullptr, fmt, args);
    ret += vfctprintf(serial::printc, nullptr, "\n", args);
    va_end(args);
    log_lock.unlock();
    return ret;
}

int warn(const char *fmt, ...)
{
    log_lock.lock();
    va_list args;
    va_start(args, fmt);
    int ret = vfctprintf(serial::printc, nullptr, "[\033[33mWARN\033[0m] ", args);
    ret += vfctprintf(serial::printc, nullptr, fmt, args);
    ret += vfctprintf(serial::printc, nullptr, "\n", args);
    va_end(args);
    log_lock.unlock();
    return ret;
}

int error(const char *fmt, ...)
{
    log_lock.lock();
    va_list args;
    va_start(args, fmt);
    int ret = vfctprintf(serial::printc, nullptr, "[\033[31mERROR\033[0m] ", args);
    ret += vfctprintf(serial::printc, nullptr, fmt, args);
    ret += vfctprintf(serial::printc, nullptr, "\n", args);
    va_end(args);
    log_lock.unlock();
    return ret;
}