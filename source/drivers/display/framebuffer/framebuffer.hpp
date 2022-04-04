// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/math.hpp>
#include <limine.h>

namespace kernel::drivers::display::framebuffer {

extern limine_framebuffer **framebuffers;
extern limine_framebuffer *main_frm;
extern uint64_t frm_count;

void putpix(uint32_t x, uint32_t y, uint32_t colour, limine_framebuffer *frm = main_frm);
void putpix(uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint64_t b, limine_framebuffer *frm = main_frm);
uint32_t getpix(uint32_t x, uint32_t y, limine_framebuffer *frm = main_frm);

void drawline(int x0, int y0, int x1, int y1, uint32_t colour, limine_framebuffer *frm = main_frm);

void drawrectangle(int x, int y, int w, int h, uint32_t colour, limine_framebuffer *frm = main_frm);
void drawfilledrectangle(int x, int y, int w, int h, uint32_t colour, limine_framebuffer *frm = main_frm);

void drawcircle(int xm, int ym, int r, uint32_t colour, limine_framebuffer *frm = main_frm);
void drawfilledcircle(int cx, int cy, int radius, uint32_t colour, limine_framebuffer *frm = main_frm);

void drawovercursor(uint8_t cursor[], point pos, uint32_t colour, bool back, limine_framebuffer *frm = main_frm);
void clearcursor(uint8_t cursor[], point pos, limine_framebuffer *frm = main_frm);

void init();
}