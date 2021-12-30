// Copyright (C) 2021  ilobilo

#include <stdint.h>
#include <stddef.h>

uint8_t flip8(uint8_t num, size_t bits)
{
    uint8_t t = num << (8 - bits);
    return t | (num >> bits);
}

uint16_t flip16(uint16_t num)
{
    uint32_t first = *reinterpret_cast<uint8_t*>(&num);
    uint32_t second = *(reinterpret_cast<uint8_t*>(&num) + 1);
    return (first << 8) | second;
}

uint32_t flip32(uint32_t num)
{
    uint32_t first = *reinterpret_cast<uint8_t*>(&num);
    uint32_t second = *(reinterpret_cast<uint8_t*>(&num) + 1);
    uint32_t third = *(reinterpret_cast<uint8_t*>(&num) + 2);
    uint32_t fourth = *(reinterpret_cast<uint8_t*>(&num) + 3);
    return (first << 24) | (second << 16) | (third >> 8) | fourth;
}

uint8_t htonb(uint8_t num, size_t bits)
{
    return flip8(num, bits);
}

uint8_t ntohb(uint8_t num, size_t bits)
{
    return flip8(num, 8 - bits);
}

uint16_t htons(uint16_t num)
{
    return flip16(num);
}

uint16_t ntohs(uint16_t num)
{
    return flip16(num);
}

uint32_t htonl(uint32_t num)
{
    return flip32(num);
}

uint32_t ntohl(uint32_t num)
{
    return flip32(num);
}