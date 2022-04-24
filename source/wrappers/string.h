// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t strlen(const char *str);

char *strcpy(char *destination, const char *source);
char *strncpy(char *destination, const char *source, size_t n);

char *strcat(char *destination, const char *source);
char *strchr(const char *str, char ch);

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);

char *strdup(const char *src);
void strrev(unsigned char *str);
char *strstr(const char *str, const char *substr);

void *memcpy(void *dest, const void *src, size_t len);
int memcmp(const void *ptr1, const void *ptr2, size_t len);
void *memset(void *dest, int ch, size_t n);
void *memmove(void *dest, const void *src, size_t len);

#ifdef __cplusplus
}
#endif