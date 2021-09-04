#pragma once
#include <stdint.h>
#include <stddef.h>

size_t strlen(const char* str);

char* strcpy(char* destination, const char* source);

char* strcat(char* destination, const char* source);

int strcmp(const char* a, const char* b);

char* strstr(const char *str, const char *substring);

void memcpy(void* dest, void* src, size_t n);

void memset(void* str, char ch, size_t n);

void memmove(void* dest, void* src, size_t n);

char* int_to_string(int num);

int string_to_int(char* str);

long oct_to_dec(int n);