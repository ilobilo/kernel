// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>

#define FLAT_PTR(PTR) (*reinterpret_cast<uintptr_t*>(PTR))
#define BYTE_PTR(PTR) (*reinterpret_cast<uint8_t*>(PTR))
#define WORD_PTR(PTR) (*reinterpret_cast<uint16_t*>(PTR))
#define DWORD_PTR(PTR) (*reinterpret_cast<uint32_t*>(PTR))
#define QWORD_PTR(PTR) (*reinterpret_cast<uint64_t*>(PTR))

void mmoutb(void *addr, uint8_t value);
void mmoutw(void *addr, uint16_t value);
void mmoutl(void *addr, uint32_t value);
void mmoutq(void *addr, uint64_t value);

uint8_t mminb(void *addr);
uint16_t mminw(void *addr);
uint32_t mminl(void *addr);
uint64_t mminq(void *addr);