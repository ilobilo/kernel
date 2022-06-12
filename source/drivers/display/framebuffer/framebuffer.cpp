// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/framebuffer/framebuffer.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/alloc.hpp>
#include <lib/log.hpp>

namespace kernel::drivers::display::framebuffer {

limine_framebuffer **framebuffers;
limine_framebuffer *main_frm;
uint64_t frm_count;

uint32_t cursorbuffer[16 * 19];
uint32_t cursorbuffersecond[16 * 19];

bool mousedrawn;

void putpix(uint64_t x, uint64_t y, uint32_t colour, limine_framebuffer *frm)
{
    *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(frm->address) + (x * 4) + (y * (frm->pitch / (frm->bpp / 8)) * 4)) = colour;
}

void putpix(uint64_t x, uint64_t y, uint32_t r, uint32_t g, uint64_t b, limine_framebuffer *frm)
{
    *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(frm->address) + (x * 4) + (y * (frm->pitch / (frm->bpp / 8)) * 4)) = (r << 16) | (g << 8) | b;
}

uint32_t getpix(uint64_t x, uint64_t y, limine_framebuffer *frm)
{
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(frm->address) + (x * 4) + (y * (frm->pitch / (frm->bpp / 8)) * 4));
}

static void drawvertline(uint64_t x, uint64_t y, uint64_t dy, uint32_t colour, limine_framebuffer *frm = main_frm)
{
    for (uint64_t i = 0; i < dy; i++) putpix(x, y + i, colour);
}

static void drawhorline(uint64_t x, uint64_t y, uint64_t dx, uint32_t colour, limine_framebuffer *frm = main_frm)
{
    for (uint64_t i = 0; i < dx; i++) putpix(x + i, y, colour);
}

static void drawdiagline(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint32_t colour, limine_framebuffer *frm = main_frm)
{
    uint64_t sdx = sign(x1);
    uint64_t sdy = sign(y1);
    uint64_t dxabs = abs(x1);
    uint64_t dyabs = abs(y1);
    uint64_t x = dyabs >> 1;
    uint64_t y = dxabs >> 1;
    uint64_t px = x0;
    uint64_t py = y0;

    if (dxabs >= dyabs)
    {
        for (size_t i = 0; i < dxabs; i++)
        {
            y += dyabs;
            if (y >= dxabs)
            {
                y -= dxabs;
                py += sdy;
            }
            px += sdx;
            putpix(px, py, colour);
        }
    }
    else
    {
        for (size_t i = 0; i < dyabs; i++)
        {
            x += dxabs;
            if (x >= dyabs)
            {
                x -= dyabs;
                px += sdx;
            }
            py += sdy;
            putpix(px, py, colour);
        }
    }
}

void drawline(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint32_t colour, limine_framebuffer *frm)
{
    if (x0 > frm->width) x0 = frm->width - 1;
    if (x1 > frm->width) x1 = frm->width - 1;
    if (y0 > frm->height) y0 = frm->height - 1;
    if (y1 > frm->height) y1 = frm->height - 1;

    if (x0 < uint64_t(-1)) x0 = -1;
    if (x1 < uint64_t(-1)) x1 = -1;
    if (y0 < uint64_t(-1)) y0 = -1;
    if (y1 < uint64_t(-1)) y1 = -1;

    uint64_t dx = x1 - x0, dy = y1 - y0;

    if (dy == 0)
    {
        drawhorline(x0, y0, dx, colour);
        return;
    }
    if (dx == 0) drawvertline(x0, y0, dy, colour);
    drawdiagline(x0, y0, dx, dy, colour);
}

void drawrectangle(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t colour, limine_framebuffer *frm)
{
    drawline(x, y, x + w, y, colour); // ¯
    drawline(x, y + h - 1, x + w, y + h - 1, colour); // _
    drawline(x, y, x, y + h, colour); // |*
    drawline(x + w - 1, y, x + w - 1, y + h, colour); // *|
}

void drawfilledrectangle(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t colour, limine_framebuffer *frm)
{
    for (uint64_t i = 0; i < h; i++) drawline(x, y + i, x + w, y + i, colour);
}

void drawcircle(uint64_t cx, uint64_t cy, uint64_t radius, uint32_t colour, limine_framebuffer *frm)
{
    uint64_t x = -radius, y = 0, err = 2 - 2 * radius;
    do
    {
        putpix(abs(cx - x), abs(cy + y), colour);
        putpix(abs(cx - y), abs(cy - x), colour);
        putpix(abs(cx + x), abs(cy - y), colour);
        putpix(abs(cx + y), abs(cy + x), colour);
        radius = err;
        if (radius > x) err += ++x * 2 + 1;
        if (radius <= y) err += ++y * 2 + 1;
    } while (x < 0);
}

void drawfilledcircle(uint64_t cx, uint64_t cy, uint64_t radius, uint32_t colour, limine_framebuffer *frm)
{
    if ((radius > cx) | (radius > cy)) { cx = radius; cy = radius; };
    uint64_t x = radius;
    uint64_t y = 0;
    uint64_t xC = 1 - (radius << 1);
    uint64_t yC = 0;
    uint64_t err = 0;

    while (x >= y)
    {
        for (uint64_t i = cx - x; i <= cx + x; i++)
        {
            putpix(i, cy + y, colour);
            putpix(i, cy - y, colour);
        }
        for (uint64_t i = cx - y; i <= cx + y; i++)
        {
            putpix(i, cy + x, colour);
            putpix(i, cy - x, colour);
        }

        y++;
        err += yC;
        yC += 2;
        if (((err << 1) + xC) > 0)
        {
            x--;
            err += xC;
            xC += 2;
        }
    }
}

void clearcursor(uint8_t cursor[], point pos, limine_framebuffer *frm)
{
    if (!mousedrawn) return;

    uint64_t xmax = 16, ymax = 19, dx = frm->width - pos.X, dy = frm->height - pos.Y;

    if (dx < 16) xmax = dx;
    if (dy < 19) ymax = dy;

    for (uint64_t y = 0; y < ymax; y++)
    {
        for (uint64_t x = 0; x < xmax; x++)
        {
            uint64_t bit = y * 16 + x;
            uint64_t byte = bit / 8;
            if ((cursor[byte] & (0b10000000 >> (x % 8))))
            {
                putpix(pos.X + x, pos.Y + y, cursorbuffer[y * 16 + x]);
            }
        }
    }
}

void drawovercursor(uint8_t cursor[], point pos, uint32_t colour, bool back, limine_framebuffer *frm)
{
    uint64_t xmax = 16, ymax = 19, dx = frm->width - pos.X, dy = frm->height - pos.Y;

    if (dx < 16) xmax = dx;
    if (dy < 19) ymax = dy;

    for (uint64_t y = 0; y < ymax; y++)
    {
        for (uint64_t x = 0; x < xmax; x++)
        {
            uint64_t bit = y * 16 + x;
            uint64_t byte = bit / 8;
            if ((cursor[byte] & (0b10000000 >> (x % 8))))
            {
                if (back) cursorbuffer[y * 16 + x] = getpix(pos.X + x, pos.Y + y);
                putpix(pos.X + x, pos.Y + y, colour);
                if (back) cursorbuffersecond[y * 16 + x] = getpix(pos.X + x, pos.Y + y);
            }
        }
    }

    mousedrawn = true;
}

void init()
{
    framebuffers = framebuffer_request.response->framebuffers;
    main_frm = framebuffers[0];
    frm_count = framebuffer_request.response->framebuffer_count;
}
}