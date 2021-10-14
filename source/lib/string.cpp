#include <drivers/display/terminal/terminal.hpp>
#include <system/mm/heap/heap.hpp>
#include <lib/string.hpp>
#include <lib/math.hpp>
#include <stdint.h>
#include <stddef.h>

size_t strlen(const char *str)
{
    size_t length = 0;
    while(str[length]) length++;
    return length;
}

char *strcpy(char *destination, const char *source)
{
    if (destination == NULL)
    {
        return NULL;
    }
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

char *strchr(const char str[], char ch)
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
    char *s = (char*)malloc(len);
    if (s == NULL) return NULL;
    return (char*)memcpy(s, (void*)src, len);
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

void *memcpy(void *dest, void *src, size_t n)
{
    size_t i;
    char *src_char = (char *)src;
    char *dest_char = (char *)dest;
    for (i = 0; i < n; i++)
    {
        dest_char[i] = src_char[i];
    }
    return dest_char;
}

int memcmp(const void *s1, const void *s2, int len)
{
    unsigned char *p = (unsigned char*)s1;
    unsigned char *q = (unsigned char*)s2;
    int charstat = 0;

    if (s1 == s2)
    {
        return charstat;
    }
    while (len > 0)
    {
        if (*p != *q)
        {
            charstat = (*p > *q) ? 1 : -1;
            break;
        }
        len--;
        p++;
        q++;
    }
    return charstat;
}

void memset(void *str, char ch, size_t n)
{
    size_t i;
    char *s = (char *)str;
    for(i = 0; i < n; i++)
    {
        s[i] = ch;
    }
}

void memmove(void *dest, void *src, size_t n)
{
    char *csrc = (char *)src;
    char *cdest = (char *)dest;
    char temp[n];
    for (size_t i = 0; i < n; i++) temp[i] = csrc[i];
    for (size_t i = 0; i < n; i++) cdest[i] = temp[i];
}

void reverse(char s[])
{
    size_t c, j, i;
    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

char *int_to_string(int num)
{
    bool isMinus = false;
    static char out[10];
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
        return out;
    }
    else return 0;
}

int string_to_int(char *str)
{
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i)
    {
        res = res * 10 + str[i] - '0';
    }
    return res;
}

long oct_to_dec(int oct)
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

char *humanify(double bytes)
{
    static char buf[10];
    int i = 0;
    const char *units[] = { "B", "KB", "MB", "GB", "TB" };
    for (i = 0; bytes > 1024; i++)
    {
        bytes /= 1024;
    }
    sprintf(buf, "%.2f%s", bytes, units[i]);
    return buf;
}
