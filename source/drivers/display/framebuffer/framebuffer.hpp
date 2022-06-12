// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/math.hpp>
#include <limine.h>

namespace kernel::drivers::display::framebuffer {

extern limine_framebuffer **framebuffers;
extern limine_framebuffer *main_frm;
extern uint64_t frm_count;

void putpix(uint64_t x, uint64_t y, uint32_t colour, limine_framebuffer *frm = main_frm);
void putpix(uint64_t x, uint64_t y, uint32_t r, uint32_t g, uint64_t b, limine_framebuffer *frm = main_frm);
uint32_t getpix(uint64_t x, uint64_t y, limine_framebuffer *frm = main_frm);

void drawline(uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint32_t colour, limine_framebuffer *frm = main_frm);

void drawrectangle(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t colour, limine_framebuffer *frm = main_frm);
void drawfilledrectangle(uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint32_t colour, limine_framebuffer *frm = main_frm);

void drawcircle(uint64_t xm, uint64_t ym, uint64_t r, uint32_t colour, limine_framebuffer *frm = main_frm);
void drawfilledcircle(uint64_t cx, uint64_t cy, uint64_t radius, uint32_t colour, limine_framebuffer *frm = main_frm);

void drawovercursor(uint8_t cursor[], point pos, uint32_t colour, bool back, limine_framebuffer *frm = main_frm);
void clearcursor(uint8_t cursor[], point pos, limine_framebuffer *frm = main_frm);

void init();
}