#include <drivers/serial/serial.hpp>
#include <drivers/drawing/drawing.hpp>
#include <include/math.hpp>
#include <main.hpp>

uint64_t frm_addr;
uint16_t frm_width;
uint16_t frm_height;
uint16_t frm_pitch;
uint16_t frm_bpp;
uint16_t frm_pixperscanline;

void putpix(uint32_t x, uint32_t y, uint32_t colour)
{
    *(uint32_t*)((uint64_t)frm_addr + (x * 4) + (y * frm_pixperscanline * 4)) = colour;
}

uint32_t getpix(uint32_t x, uint32_t y)
{
    return *(uint32_t*)((uint64_t)frm_addr + (x * 4) + (y * frm_pixperscanline * 4));
}

void drawvertline(int x, int y, int dy, uint32_t colour)
{
    for (int i = 0; i < dy; i++)
    {
        putpix(x, y + i, colour);
    }
}

void drawhorline(int x, int y, int dx, uint32_t colour)
{
    for (int i = 0; i < dx; i++)
    {
        putpix(x + i, y, colour);
    }
}

void drawdiagline(int x0, int y0, int x1, int y1, uint32_t colour)
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

void drawline(int x0, int y0, int x1, int y1, uint32_t colour)
{
    if (x0 > frm_width) x0 = frm_width - 1;
    if (x1 > frm_width) x1 = frm_width - 1;
    if (y0 > frm_height) y0 = frm_height - 1;
    if (y1 > frm_height) y1 = frm_height - 1;

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
    if (dx == 0)
    {
        drawvertline(x0, y0, dy, colour);
    }
    drawdiagline(x0, y0, dx, dy, colour);
}

void drawrectangle(int x, int y, int w, int h, uint32_t colour)
{
    drawline(x, y, x + w, y, colour); // ¯
    drawline(x, y + h - 1, x + w, y + h - 1, colour); // _
    drawline(x, y, x, y + h, colour); // |*
    drawline(x + w - 1, y, x + w - 1, y + h, colour); // *|
}

void drawfilledrectangle(int x, int y, int w, int h, uint32_t colour)
{
    for (int i = 0; i < h; i++)
    {
        drawline(x, y + i, x + w, y + i, colour);
    }
}

void drawcircle(int cx, int cy, int radius, uint32_t colour)
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

void drawfilledcircle(int cx, int cy, int radius, uint32_t colour)
{
    if ((radius > cx) | (radius > cy)) { cx = radius; cy = radius; };
    int x = radius;
    int y = 0;
    int xC = 1 - (radius << 1);
    int yC = 0;
    int err = 0;

    while (x >= y)
    {
        for (uint32_t i = cx - x; i <= cx + x; i++)
        {
            putpix(i, cy + y, colour);
            putpix(i, cy - y, colour);
        }
        for (uint32_t i = cx - y; i <= cx + y; i++)
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

void drawing_init()
{
    serial_info("Initializing drawing functions\n");

    frm_addr = frm_tag->framebuffer_addr;
    frm_width = frm_tag->framebuffer_width;
    frm_height = frm_tag->framebuffer_height;
    frm_pitch = frm_tag->framebuffer_pitch;
    frm_bpp = frm_tag->framebuffer_bpp;
    frm_pixperscanline = frm_pitch / 4;

    serial_info("Initialized drawing functions\n\n");
}
