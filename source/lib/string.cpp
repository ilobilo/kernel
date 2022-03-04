// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <lib/memory.hpp>
#include <lib/string.hpp>
#include <lib/alloc.hpp>
#include <lib/math.hpp>
#include <stdint.h>
#include <stddef.h>

size_t strlen(const char *str)
{
    if (str == nullptr) return 0;
    size_t length = 0;
    while(str[length]) length++;
    return length;
}

char *strcpy(char *destination, const char *source)
{
    if (destination == nullptr) return nullptr;
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
    if (destination == nullptr) return nullptr;
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
    if (destination == nullptr) return nullptr;
    char *ptr = destination + strlen(destination);
    while (*source != '\0') *ptr++ = *source++;
    return destination;
}

char *strchr(const char *str, char ch)
{
    if (str == nullptr || ch == 0) return nullptr;
    while (*str && *str != ch) ++str;
    return const_cast<char*>(ch == *str ? str : nullptr);
}

int strcmp(const char *a, const char *b)
{
    if (a == nullptr || b == nullptr) return 1;
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return *a - *b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    if (a == nullptr || b == nullptr) return 1;
    for (size_t i = 0; i < n; i++)
    {
        if (a[i] != b[i]) return 1;
    }
    return 0;
}

char *strrm(char *str, const char *substr)
{
    if (str == nullptr || substr == nullptr) return nullptr;
    char *p, *q, *r;
    if (*substr && (q = r = strstr(str, substr)) != nullptr)
    {
        size_t len = strlen(substr);
        while ((r = strstr(p = r + len, substr)) != nullptr)
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
    char *s = static_cast<char*>(malloc(len));
    if (s == nullptr) return nullptr;
    return static_cast<char*>(memcpy(s, const_cast<void*>(reinterpret_cast<const void*>(src)), len));
}

static char** _strsplit(const char* s, const char* delim, size_t* nb)
{
    void *data;
    char *_s = const_cast<char*>(s);
    const char **ptrs;
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
    ptrs = static_cast<const char**>(malloc(ptrsSize + sLen + 1));
    data = ptrs;
    if (data)
    {
        *ptrs = _s = strcpy((static_cast<char*>(data)) + ptrsSize, s);
        if (nbWords > 1)
        {
            while ((_s = strstr(_s, delim)))
            {
                *_s = '\0';
                _s += delimLen;
                *++ptrs = _s;
            }
        }
        *++ptrs = nullptr;
    }
    if (nb) *nb = data ? nbWords : 0;
    return static_cast<char**>(data);
}

char** strsplit(const char* s, const char* delim)
{
    return _strsplit(s, delim, nullptr);
}

char** strsplit_count(const char* s, const char* delim, size_t &nb)
{
    return _strsplit(s, delim, &nb);
}

char *strstr(const char *str, const char *substr)
{
    const char *a = str, *b = substr;
    while (true)
    {
        if (!*b) return (char *)str;
        if (!*a) return nullptr;
        if (*a++ != *b++)
        {
            a = ++str;
            b = substr;
        }
    }
}

int lstrstr(const char *str, const char *substr, int skip)
{
    int count = 0;
    const char *a = str, *b = substr;
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
            b = substr;
        }
        if (*str == '\n') count++;
    }
    return -1;
}

char *getline(const char *str, const char *substr, char *buffer, int skip)
{
    int i = 0;
    const char *strbck = str;
    const char *a = str, *b = substr;
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
                memcpy(buffer, const_cast<void*>(reinterpret_cast<const void*>(&strbck[i])), size);
                buffer[size] = 0;
                return buffer;
            }
            else skip--;
        }
        if (!*a) return 0;
        if (*a++ != *b++)
        {
            a = ++str;
            b = substr;
            i++;
        }
    }
    return 0;
}

char *reverse(char *str)
{
    for (size_t i = 0, j = strlen(str) - 1; i < j; i++, j--)
    {
        char c = str[i];
        str[i] = str[j];
        str[j] = c;
    }
    return str;
}

bool isspace(char c)
{
    if (c == ' ') return true;
    return false;
}

bool isempty(char *str)
{
    if (str == nullptr || strlen(str) == 0) return true;
    while (*str != '\0')
    {
        if (!isspace(*str)) return false;
        str++;
    }
    return true;
}

char tolower(char c)
{
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

char toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

int tonum(char c)
{
    c = toupper(c);
    return (c) ? c - 64 : -1;
}

char str[2];
char *tostr(char c)
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