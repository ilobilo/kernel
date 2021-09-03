#pragma once
#include <stdint.h>
#include <stddef.h>

size_t strlen(const char* str);

char* strcpy(char* destination, const char* source);

char* strcat(char* destination, const char* source);

void memcpy(void *dest, void *src, size_t n);

void memset(void* str, char ch, size_t n);

void memmove(void *dest, void *src, size_t n);

void int_to_string(int n, char str[]);