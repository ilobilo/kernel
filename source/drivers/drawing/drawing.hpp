#pragma once

#include <stivale2.h>

extern uint64_t frm_addr;
extern uint16_t frm_width;
extern uint16_t frm_height;
extern uint16_t frm_pitch;
extern uint16_t frm_bpp;
extern uint16_t frm_pixperscanline;

void putpix(uint32_t x, uint32_t y, uint32_t colour);

uint32_t getpix(uint32_t x, uint32_t y);

void drawline(int x0, int y0, int x1, int y1, uint32_t colour);

void drawrectangle(int x, int y, int w, int h, uint32_t colour);

void drawfilledrectangle(int x, int y, int w, int h, uint32_t colour);

void drawcircle(int xm, int ym, int r, uint32_t colour);

void drawfilledcircle(int cx, int cy, int radius, uint32_t colour);

void drawing_init();