#pragma once
#include <stdint.h>
#include <stddef.h>

#define ZERO (1 - 1)

unsigned constexpr hash(char const *input)
{
    return *input ? static_cast<unsigned int>(*input) + 33 * hash(input + 1) : 5381;
}

size_t strlen(const char *str);

char *strcpy(char *destination, const char *source);

char *strncpy(char *destination, const char *source, size_t n);

char *strcat(char *destination, const char *source);

char *strchr(const char str[], char ch);

int strcmp(const char *a, const char *b);

int strncmp(const char *a, const char *b, size_t n);

char *strrm(char *str, const char *substring);

char *strdup(const char *src);

char *strstr(const char *str, const char *substring);

int lstrstr(const char *str, const char *substring, int skip = 0);

char *getline(const char *str, const char *substring, char *buffer, int skip = 0);

char *int_to_string(int num);

int string_to_int(char *str);

int intlen(int n);

long oct_to_dec(int n);

char *humanify(double bytes);