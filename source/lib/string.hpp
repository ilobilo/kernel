// Copyright (C) 2021  ilobilo

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
char *strchr(const char *str, char ch);

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);

char *strrm(char *str, const char *substring);
char *strdup(const char *src);

char** strsplit(const char* s, const char* delim);
char** strsplit_count(const char* s, const char* delim, size_t &nb);

char *strstr(const char *str, const char *substring);
int lstrstr(const char *str, const char *substring, int skip = 0);

char *getline(const char *str, const char *substring, char *buffer, int skip = 0);

char char2low(char c);
char char2up(char c);
int char2num(char c);
char *char2str(char c);

char *int2string(int num);
int string2int(const char *str);