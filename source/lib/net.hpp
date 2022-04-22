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

template<typename type>
struct bigendian
{
    type value;

    bigendian() : value(0) { }
    bigendian(const type &_value) : value(_value) { }

    bigendian<type> &operator=(const type &_value)
    {
        switch (sizeof(type))
        {
            case sizeof(uint64_t):
                this->value = htonq(_value);
                break;
            case sizeof(uint32_t):
                this->value = htonl(_value);
                break;
            case sizeof(uint16_t):
                this->value = htons(_value);
                break;
            case sizeof(uint8_t):
                this->value = _value;
                break;
        }
        return *this;
    }

    inline constexpr operator type() const
    {
        switch (sizeof(type))
        {
            case sizeof(uint64_t):
                return htonq(this->value);
            case sizeof(uint32_t):
                return htonl(this->value);
            case sizeof(uint16_t):
                return htons(this->value);
            case sizeof(uint8_t):
                return this->value;
        }
        return 0;
    }
};

struct [[gnu::packed]] ipv4addr
{
    uint8_t addr[4];

    ipv4addr() { };
    ipv4addr(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth)
    {
        this->addr[0] = first;
        this->addr[1] = second;
        this->addr[2] = third;
        this->addr[3] = fourth;
    }

    ipv4addr(uint8_t *addr)
    {
        if (addr == nullptr) return;
        *this->addr = *addr;
    }

    ipv4addr &operator=(const uint8_t *addr)
    {
        if (addr == nullptr) return *this;
        *this->addr = *addr;
        return *this;
    }

    bool operator==(uint8_t *addr)
    {
        return !memcmp(this->addr, addr, 4);
    }

    bool operator==(ipv4addr &addr)
    {
        return !memcmp(this->addr, addr.addr, 4);
    }

    uint8_t &operator[](size_t pos)
    {
        return this->addr[pos];
    }
};

struct [[gnu::packed]] macaddr
{
    uint8_t addr[6];

    macaddr() { };
    macaddr(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth, uint8_t fifth, uint8_t sixth)
    {
        this->addr[0] = first;
        this->addr[1] = second;
        this->addr[2] = third;
        this->addr[3] = fourth;
        this->addr[4] = fifth;
        this->addr[5] = sixth;
    }

    macaddr(uint8_t *addr)
    {
        if (addr == nullptr) return;
        *this->addr = *addr;
    }

    macaddr &operator=(uint8_t *addr)
    {
        if (addr == nullptr) return *this;
        *this->addr = *addr;
        return *this;
    }

    bool operator==(uint8_t *addr)
    {
        return !memcmp(this->addr, addr, 6);
    }

    bool operator==(macaddr &addr)
    {
        return !memcmp(this->addr, addr.addr, 6);
    }

    uint8_t &operator[](size_t pos)
    {
        return this->addr[pos];
    }
};

static inline bigendian<uint16_t> checksum(void *addr, size_t size)
{
    uint16_t *ptr = static_cast<uint16_t*>(addr);
    size_t count = size;
    uint32_t checksum = 0;

    while (count >= 2)
    {
        checksum += *ptr++;
        count -= 2;
    }

    checksum = (checksum & 0xFFFF) + (checksum >> 16);
    if (checksum > UINT16_MAX) checksum += 1;

    bigendian<uint16_t> ret;
    ret.value = ~checksum;
    return ret;
}