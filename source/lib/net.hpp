// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>
#include <cstddef>

static inline uint16_t htons(uint16_t num)
{
    return __builtin_bswap16(num);
}
static inline uint16_t ntohs(uint16_t num)
{
    return __builtin_bswap16(num);
}

static inline uint32_t htonl(uint32_t num)
{
    return __builtin_bswap32(num);
}
static inline uint32_t ntohl(uint32_t num)
{
    return __builtin_bswap32(num);
}

static inline uint64_t htonq(uint64_t num)
{
    return __builtin_bswap64(num);
}
static inline uint64_t ntohq(uint64_t num)
{
    return __builtin_bswap64(num);
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

static inline uint16_t checksum(void *addr, size_t size, size_t start = 0)
{
    uint32_t sum = start;
    sum += sum16bits(addr, size);

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

template<typename type>
struct bigendian
{
    type value;

    bigendian() : value(0) { }
    bigendian(const type &_value) : value(_value) { }

    bigendian<type> &operator=(const type &_value)
    {
        if (sizeof(type) == sizeof(uint64_t)) this->value = htonq(_value);
        else if (sizeof(type) == sizeof(uint32_t)) this->value = htonl(_value);
        else if (sizeof(type) == sizeof(uint16_t)) this->value = htons(_value);
        else if (sizeof(type) == sizeof(uint8_t)) this->value = _value;
        return *this;
    }

    inline constexpr operator type() const
    {
        if (sizeof(type) == sizeof(uint64_t)) return htonq(this->value);
        else if (sizeof(type) == sizeof(uint32_t)) return htonl(this->value);
        else if (sizeof(type) == sizeof(uint16_t)) return htons(this->value);
        else if (sizeof(type) == sizeof(uint8_t)) return this->value;
        return 0;
    }
};