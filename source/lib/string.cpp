// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/mm/heap/heap.hpp>
#include <lib/memory.hpp>
#include <lib/string.hpp>
#include <lib/math.hpp>
#include <stdint.h>
#include <stddef.h>

using namespace kernel::system::mm;

size_t strlen(const char *str)
{
    size_t length = 0;
    while(str[length]) length++;
    return length;
}

char *strcpy(char *destination, const char *source)
{
    if (destination == NULL) return NULL;
    char *ptr = destination;
    while (*source != '\0')
    {
        *destination = *source;
        destination++;
        source++;
    }
    *destination = '\0';
    return ptr;
}

char *strncpy(char *destination, const char *source, size_t n)
{
    if (!destination) return NULL;
    char *ptr = destination;
    for (size_t i = 0; i < n && *source != '\0'; i++)
    {
        *destination = *source;
        destination++;
        source++;
    }
    *destination = '\0';
    return ptr;
}

char *strcat(char *destination, const char *source)
{
    char *ptr = destination + strlen(destination);
    while (*source != '\0')
    {
        *ptr++ = *source++;
    }
    *ptr = '\0';
    return destination;
}

char *strchr(const char *str, char ch)
{
    while (*str && *str != ch ) ++str;
    return (char *)(ch == *str ? str : NULL);
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return (int)(unsigned char)(*a) - (int)(unsigned char)(*b);
}

int strncmp(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (a[i] != b[i]) return 1;
    }
    return 0;
}

char *strrm(char *str, const char *substring)
{
    char *p, *q, *r;
    if (*substring && (q = r = strstr(str, substring)) != NULL)
    {
        size_t len = strlen(substring);
        while ((r = strstr(p = r + len, substring)) != NULL)
        {
            while (p < r) *q++ = *p++;
        }
        while ((*q++ = *p++) != '\0') continue;
    }
    return str;
}

char *strdup(const char *src)
{
    size_t len = strlen(src) + 1;
    char *s = (char*)heap::malloc(len);
    if (s == NULL) return NULL;
    return (char*)memcpy(s, (void*)src, len);
}

static char** _strsplit(const char* s, const char* delim, size_t* nb)
{
    void* data;
    char* _s = (char*)s;
    const char** ptrs;
    size_t ptrsSize;
    size_t nbWords = 1;
    size_t sLen = strlen(s);
    size_t delimLen = strlen(delim);

    while ((_s = strstr(_s, delim)))
    {
        _s += delimLen;
        ++nbWords;
    }
    ptrsSize = (nbWords + 1) * sizeof(char*);
    ptrs = (const char**)heap::malloc(ptrsSize + sLen + 1);
    data = ptrs;
    if (data)
    {
        *ptrs = _s = strcpy(((char*)data) + ptrsSize, s);
        if (nbWords > 1)
        {
            while (( _s = strstr(_s, delim)))
            {
                *_s = '\0';
                _s += delimLen;
                *++ptrs = _s;
            }
        }
        *++ptrs = NULL;
    }
    if (nb) *nb = data ? nbWords : 0;
    return (char**)data;
}

char** strsplit(const char* s, const char* delim)
{
    return _strsplit(s, delim, NULL);
}

char** strsplit_count(const char* s, const char* delim, size_t &nb)
{
    return _strsplit(s, delim, &nb);
}

char *strstr(const char *str, const char *substring)
{
    const char *a = str, *b = substring;
    while (true)
    {
        if (!*b) return (char *)str;
        if (!*a) return NULL;
        if (*a++ != *b++)
        {
            a = ++str;
            b = substring;
        }
    }
}

int lstrstr(const char *str, const char *substring, int skip)
{
    int count = 0;
    const char *a = str, *b = substring;
    while (true)
    {
        if (!*b)
        {
            if (skip == 0) return count;
            else skip--;
        }
        if (!*a) return -1;
        if (*a++ != *b++)
        {
            a = ++str;
            b = substring;
        }
        if (*str == '\n') count++;
    }
    return -1;
}

char *getline(const char *str, const char *substring, char *buffer, int skip)
{
    int i = 0;
    const char *strbck = str;
    const char *a = str, *b = substring;
    while (true)
    {
        if (!*b)
        {
            if (skip == 0)
            {
                int t = i;
                while (strbck[i - 1] != '\n') i--;
                while (strbck[t] != '\n') t++;
                int size = t - i;
                memcpy(buffer, (void*)&strbck[i], size);
                buffer[size] = 0;
                return buffer;
            }
            else skip--;
        }
        if (!*a) return 0;
        if (*a++ != *b++)
        {
            a = ++str;
            b = substring;
            i++;
        }
    }
    return 0;
}

char *reverse(char s[])
{
    size_t c, j, i;
    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
    return s;
}

char char2low(char c)
{
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

char char2up(char c)
{
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

int char2num(char c)
{
    c = char2up(c);
    return (c) ? c - 64 : -1;
}

char str[2];
char *char2str(char c)
{
    str[0] = c;
    return str;
}

char *int2string(int num)
{
    bool isMinus = false;
    char out[10];
    int g = 0;
    if (num != 0)
    {
        char temp[10];
        int i = 0;
        if (num < 0)
        {
            isMinus = true;
            num = -num;
        }
        if (num > 0);
        else
        {
            temp[i++] = '8';
            num = -(num / 10);
        }
        while (num > 0)
        {
            temp[i++] = num % 10 + '0';
            num /= 10;
        }
        if (isMinus)
        {
            out[g] = '-';
            g++;
        }
        while (--i >= 0)
        {
            out[g] = temp[i];
            g++;
        }
        return strdup(out);
    }
    else return 0;
}

int string2int(const char *str)
{
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i)
    {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

uint64_t oct2dec(int oct)
{
    int dec = 0, temp = 0;
    while (oct != 0)
    {
        dec = dec + (oct % 10) * pow(8, temp);
        temp++;
        oct = oct / 10;
    }
    return dec;
}

int intlen(int n)
{
    int digits = 0;
    if (n <= 0)
    {
        n = -n;
        ++digits;
    }
    while (n)
    {
        n /= 10;
        ++digits;
    }
    return digits;
}