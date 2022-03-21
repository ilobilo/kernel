// Copyright (C) 2021-2022  ilobilo

#include <cstdint>
#include <cstddef>

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

static uint32_t sum16bits(void *addr, size_t size)
{
    uint32_t sum = 0;
    uint16_t *ptr = static_cast<uint16_t*>(addr);

    while (size > 1)
    {
        sum += *ptr++;
        size -= 2;
    }
    if (size > 0) sum += *reinterpret_cast<uint8_t*>(ptr);
    return sum;
}

uint16_t checksum(void *addr, size_t size, size_t start)
{
    uint32_t sum = start;
    sum += sum16bits(addr, size);

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}