#include <main.hpp>
#include <stdint.h>
#include <stddef.h>

namespace kernel::lib::memory {

uint64_t getmemsize()
{
    static uint64_t meminbytes = 0;
    if (meminbytes > 0) return meminbytes;

    for (uint64_t i = 0; i < mmap_tag->entries; i++)
    {
        if (mmap_tag->memmap[i].type != STIVALE2_MMAP_USABLE) continue;

        meminbytes += mmap_tag->memmap[i].length;
    }

    return meminbytes;
}


void *memcpy(void *dest, void *src, size_t n)
{
    long d0, d1, d2; 
    asm volatile (
        "rep ; movsq\n\t movq %4,%%rcx\n\t""rep ; movsb\n\t": "=&c" (d0),
        "=&D" (d1),
        "=&S" (d2): "0" (n >> 3), 
        "g" (n & 7), 
        "1" (dest),
        "2" (src): "memory"
    );
    return dest;
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

}