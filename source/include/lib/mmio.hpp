// Copyright (C) 2021  ilobilo

#pragma once

#include <stdint.h>

#define BYTE_PTR(PTR) (*((uint8_t *)(PTR)))
#define WORD_PTR(PTR) (*((uint16_t *)(PTR)))
#define DWORD_PTR(PTR) (*((uint32_t *)(PTR)))
#define QWORD_PTR(PTR) (*((uint64_t *)(PTR)))

void mmoutb(void *addr, uint8_t value);
void mmoutw(void *addr, uint16_t value);
void mmoutd(void *addr, uint32_t value);
void mmoutq(void *addr, uint64_t value);

uint8_t mminb(void *addr);
uint16_t mminw(void *addr);
uint32_t mmind(void *addr);
uint64_t mminq(void *addr);