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

void putpix(uint32_t x, uint32_t y, uint32_t colour, limine_framebuffer *frm)
{
    *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(frm->address) + (x * 4) + (y * (frm->pitch / (frm->bpp / 8)) * 4)) = colour;
}

void putpix(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint64_t b, limine_framebuffer *frm)
{
    *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(frm->address) + (x * 4) + (y * (frm->pitch / (frm->bpp / 8)) * 4)) = (r << 16) | (g << 8) | b;
}

uint32_t getpix(uint32_t x, uint32_t y, limine_framebuffer *frm)
{
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint64_t>(frm->address) + (x * 4) + (y * (frm->pitch / (frm->bpp / 8)) * 4));
}

static void drawvertline(int x, int y, int dy, uint32_t colour, limine_framebuffer *frm = main_frm)
{
    for (int i = 0; i < dy; i++) putpix(x, y + i, colour);
}

static void drawhorline(int x, int y, int dx, uint32_t colour, limine_framebuffer *frm = main_frm)
{
    for (int i = 0; i < dx; i++) putpix(x + i, y, colour);
}

static void drawdiagline(int x0, int y0, int x1, int y1, uint32_t colour, limine_framebuffer *frm = main_frm)
{
    int i;
    int sdx = sign(x1);
    int sdy = sign(y1);
    int dxabs = abs(x1);
    int dyabs = abs(y1);
    int x = dyabs >> 1;
    int y = dxabs >> 1;
    int px = x0;
    int py = y0;

    if (dxabs >= dyabs)
    {
        for (i = 0; i < dxabs; i++)
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
        for (i = 0; i < dyabs; i++)
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

void drawline(int x0, int y0, int x1, int y1, uint32_t colour, limine_framebuffer *frm)
{
    if (x0 > frm->width) x0 = frm->width - 1;
    if (x1 > frm->width) x1 = frm->width - 1;
    if (y0 > frm->height) y0 = frm->height - 1;
    if (y1 > frm->height) y1 = frm->height - 1;

    if (x0 < -1) x0 = -1;
    if (x1 < -1) x1 = -1;
    if (y0 < -1) y0 = -1;
    if (y1 < -1) y1 = -1;

    int dx = x1 - x0, dy = y1 - y0;

    if (dy == 0)
    {
        drawhorline(x0, y0, dx, colour);
        return;
    }
    if (dx == 0) drawvertline(x0, y0, dy, colour);
    drawdiagline(x0, y0, dx, dy, colour);
}

void drawrectangle(int x, int y, int w, int h, uint32_t colour, limine_framebuffer *frm)
{
    drawline(x, y, x + w, y, colour); // ¯
    drawline(x, y + h - 1, x + w, y + h - 1, colour); // _
    drawline(x, y, x, y + h, colour); // |*
    drawline(x + w - 1, y, x + w - 1, y + h, colour); // *|
}

void drawfilledrectangle(int x, int y, int w, int h, uint32_t colour, limine_framebuffer *frm)
{
    for (int i = 0; i < h; i++) drawline(x, y + i, x + w, y + i, colour);
}

void drawcircle(int cx, int cy, int radius, uint32_t colour, limine_framebuffer *frm)
{
    int x = -radius, y = 0, err = 2 - 2 * radius;
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

void drawfilledcircle(int cx, int cy, int radius, uint32_t colour, limine_framebuffer *frm)
{
    if ((radius > cx) | (radius > cy)) { cx = radius; cy = radius; };
    int x = radius;
    int y = 0;
    int xC = 1 - (radius << 1);
    int yC = 0;
    int err = 0;

    while (x >= y)
    {
        for (int i = cx - x; i <= cx + x; i++)
        {
            putpix(i, cy + y, colour);
            putpix(i, cy - y, colour);
        }
        for (int i = cx - y; i <= cx + y; i++)
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

    int xmax = 16, ymax = 19, dx = frm->width - pos.X, dy = frm->height - pos.Y;

    if (dx < 16) xmax = dx;
    if (dy < 19) ymax = dy;

    for (int y = 0; y < ymax; y++)
    {
        for (int x = 0; x < xmax; x++)
        {
            int bit = y * 16 + x;
            int byte = bit / 8;
            if ((cursor[byte] & (0b10000000 >> (x % 8))))
            {
                putpix(pos.X + x, pos.Y + y, cursorbuffer[y * 16 + x]);
            }
        }
    }
}

void drawovercursor(uint8_t cursor[], point pos, uint32_t colour, bool back, limine_framebuffer *frm)
{
    int xmax = 16, ymax = 19, dx = frm->width - pos.X, dy = frm->height - pos.Y;

    if (dx < 16) xmax = dx;
    if (dy < 19) ymax = dy;

    for (int y = 0; y < ymax; y++)
    {
        for (int x = 0; x < xmax; x++)
        {
            int bit = y * 16 + x;
            int byte = bit / 8;
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