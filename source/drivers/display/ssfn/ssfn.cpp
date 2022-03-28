// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#define SSFN_CONSOLEBITMAP_TRUECOLOR
#define SSFN_CONSOLEBITMAP_CONTROL
#define __THROW
#include <ssfn.h>

namespace kernel::drivers::display::ssfn {

uint32_t bgcolour;
uint32_t fgcolour = 0xFFFFFF;
point pos = { 0, 0 };

void putc(char c, void *arg)
{
    ssfn_putc(c);
}

void setcolour(uint64_t fg, uint64_t bg)
{
    bgcolour = bg;
    fgcolour = fg;

    ssfn_dst.fg = fgcolour;
    ssfn_dst.bg = bgcolour;
}

void setpos(uint64_t x, uint64_t y)
{
    pos.X = ssfn_dst.x = x * 8;
    pos.Y = ssfn_dst.y = y * 16;
}

void resetpos()
{
    pos.X = ssfn_dst.x = 0;
    pos.Y = ssfn_dst.y = 0;
}

void printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfctprintf(&putc, nullptr, fmt, args);
    va_end(args);
}

void init(uint64_t sfn)
{
    ssfn_src = reinterpret_cast<ssfn_font_t*>(sfn);

    ssfn_dst.ptr = reinterpret_cast<uint8_t*>(framebuffer::main_frm->address);
    ssfn_dst.w = framebuffer::main_frm->width;
    ssfn_dst.h = framebuffer::main_frm->height;
    ssfn_dst.p = framebuffer::main_frm->pitch;
    ssfn_dst.x = ssfn_dst.y = pos.X = pos.Y = 0;
    ssfn_dst.fg = fgcolour;
}
}