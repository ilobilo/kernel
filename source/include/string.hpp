#pragma once
#include <stdint.h>
#include <stddef.h>

uint32_t strlen(const char* str);

char* strcpy(char* destination, const char* source);

char* strcat(char* destination, const char* source);

void memcpy(void *dest, void *src, size_t n);

void memset(void* str, char ch, size_t n);

void memmove(void *dest, void *src, size_t n);

int itoa(char*, int);
