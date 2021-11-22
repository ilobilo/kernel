// Copyright (C) 2021  ilobilo

#include <lib/mmio.hpp>

void mmoutb(void *addr, uint8_t value)
{
    asm volatile("mov %1, %0\n\t" : "=m"(BYTE_PTR(addr)) : "r"(value) : "memory");
}

void mmoutw(void *addr, uint16_t value)
{
    asm volatile("mov %1, %0\n\t" : "=m"(WORD_PTR(addr)) : "r"(value) : "memory");
}

void mmoutd(void *addr, uint32_t value)
{
    asm volatile("mov %1, %0\n\t" : "=m"(DWORD_PTR(addr)) : "r"(value) : "memory");
}

void mmoutq(void *addr, uint64_t value)
{
    asm volatile("mov %1, %0\n\t" : "=m"(QWORD_PTR(addr)) : "r"(value) : "memory");
}

uint8_t mminb(void *addr)
{
	uint8_t ret;
	asm volatile("mov %1, %0\n\t" : "=r"(ret) : "m"(BYTE_PTR(addr)) : "memory");
	return ret;
}

uint16_t mminw(void *addr)
{
	uint16_t ret;
	asm volatile("mov %1, %0\n\t" : "=r"(ret) : "m"(WORD_PTR(addr)) : "memory");
	return ret;
}

uint32_t mmind(void *addr)
{
	uint32_t ret;
	asm volatile("mov %1, %0\n\t" : "=r"(ret) : "m"(DWORD_PTR(addr)) : "memory");
	return ret;
}

uint64_t mminq(void *addr)
{
	uint64_t ret;
	asm volatile("mov %1, %0\n\t" : "=r"(ret) : "m"(QWORD_PTR(addr)) : "memory");
	return ret;
}