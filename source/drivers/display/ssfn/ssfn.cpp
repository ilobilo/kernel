// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#define SSFN_CONSOLEBITMAP_TRUECOLOR
#define SSFN_CONSOLEBITMAP_CONTROL
#include <ssfn.h>

namespace kernel::drivers::display::ssfn {

uint64_t bgcolour;
uint64_t fgcolour = 0xFFFFFF;
point pos;

void printc(char c, void *arg)
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
    x++;
    y++;
    pos.X = x * 8 - 8;
    pos.Y = y * 16 - 16;

    ssfn_dst.x = pos.X;
    ssfn_dst.y = pos.Y;
}

void setppos(uint64_t x, uint64_t y)
{
    pos.X = x;
    pos.Y = y;

    ssfn_dst.x = pos.X;
    ssfn_dst.y = pos.Y;
}

void printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, fmt, args);
    va_end(args);

    pos.X = ssfn_dst.x / 8;
    pos.Y = ssfn_dst.y / 16;
}

void printfat(uint64_t x, uint64_t y, const char *fmt, ...)
{
    x++;
    y++;
    ssfn_dst.x = x * 8 - 8;
    ssfn_dst.y = y * 16 - 16;

    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, fmt, args);
    va_end(args);

    pos.X = ssfn_dst.x / 8;
    pos.Y = ssfn_dst.y / 16;
}

void pprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, fmt, args);
    va_end(args);

    pos.X = ssfn_dst.x;
    pos.Y = ssfn_dst.y;
}

void pprintfat(uint64_t x, uint64_t y, const char *fmt, ...)
{
    ssfn_dst.x = x;
    ssfn_dst.y = y;

    va_list args;
    va_start(args, fmt);
    vfctprintf(&printc, nullptr, fmt, args);
    va_end(args);

    pos.X = ssfn_dst.x;
    pos.Y = ssfn_dst.y;
}

extern "C" uint64_t _binary_font_sfn_start;
void init()
{
    ssfn_src = reinterpret_cast<ssfn_font_t*>(&_binary_font_sfn_start);

    ssfn_dst.ptr = reinterpret_cast<uint8_t*>(framebuffer::frm_addr);
    ssfn_dst.w = framebuffer::frm_width;
    ssfn_dst.h = framebuffer::frm_height;
    ssfn_dst.p = framebuffer::frm_pitch;
    ssfn_dst.x = ssfn_dst.y = 0;
    pos.X = 0;
    pos.Y = 0;
    ssfn_dst.fg = fgcolour;
    ssfn_dst.bg = bgcolour;
}
}

// // Copyright (C) 2021-2022  ilobilo

// #include <drivers/display/framebuffer/framebuffer.hpp>
// #include <drivers/display/terminal/terminal.hpp>
// #define SSFN_CONSOLEBITMAP_TRUECOLOR
// #define SSFN_IMPLEMENTATION
// #define __THROW
// #include <ssfn.h>

// namespace kernel::drivers::display::ssfn {

// uint32_t bgcolour;
// uint32_t fgcolour = 0xFFFFFF;
// point pos = { 0, 0 };

// SSFN::Font font = SSFN::Font();
// ssfn_buf_t buf
// {
//     .ptr = reinterpret_cast<uint8_t*>(framebuffer::frm_addr),
//     .w = static_cast<int16_t>(framebuffer::frm_width),
//     .h = static_cast<int16_t>(framebuffer::frm_height),
//     .p = framebuffer::frm_pitch,
//     .x = static_cast<int16_t>(0),
//     .y = static_cast<int16_t>(0),
//     .fg = fgcolour
// };

// void printc(char c, void *arg)
// {
//     error("%s", font.ErrorStr(font.Render(&buf, tostr(c))).c_str());
// }

// void setcolour(uint64_t fg, uint64_t bg)
// {
//     bgcolour = bg;
//     fgcolour = fg;

//     buf.fg = fgcolour;
//     buf.bg = bgcolour;
// }

// void setpos(uint64_t x, uint64_t y)
// {
//     x++;
//     y++;
//     pos.X = x * 8 - 8;
//     pos.Y = y * 16 - 16;

//     buf.x = pos.X;
//     buf.y = pos.Y;
// }

// void setppos(uint64_t x, uint64_t y)
// {
//     pos.X = x;
//     pos.Y = y;

//     buf.x = pos.X;
//     buf.y = pos.Y;
// }

// void printf(const char *fmt, ...)
// {
//     va_list args;
//     va_start(args, fmt);
//     vfctprintf(&printc, nullptr, fmt, args);
//     va_end(args);

//     pos.X = buf.x / 8;
//     pos.Y = buf.y / 16;
// }

// void printfat(uint64_t x, uint64_t y, const char *fmt, ...)
// {
//     x++;
//     y++;
//     buf.x = x * 8 - 8;
//     buf.y = y * 16 - 16;

//     va_list args;
//     va_start(args, fmt);
//     vfctprintf(&printc, nullptr, fmt, args);
//     va_end(args);

//     pos.X = buf.x / 8;
//     pos.Y = buf.y / 16;
// }

// void pprintf(const char *fmt, ...)
// {
//     va_list args;
//     va_start(args, fmt);
//     vfctprintf(&printc, nullptr, fmt, args);
//     va_end(args);

//     pos.X = buf.x;
//     pos.Y = buf.y;
// }

// void pprintfat(uint64_t x, uint64_t y, const char *fmt, ...)
// {
//     buf.x = x;
//     buf.y = y;

//     va_list args;
//     va_start(args, fmt);
//     vfctprintf(&printc, nullptr, fmt, args);
//     va_end(args);

//     pos.X = buf.x;
//     pos.Y = buf.y;
// }

// extern "C" uint64_t _binary_font_sfn_start;
// [[gnu::constructor]] void init()
// {
//     font.Load(reinterpret_cast<char*>(&_binary_font_sfn_start));
// }
// }