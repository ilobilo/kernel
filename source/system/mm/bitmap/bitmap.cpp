#include <system/mm/bitmap/bitmap.hpp>

bool Bitmap::operator[](uint64_t index)
{
    return Get(index);
}

bool Bitmap::Get(uint64_t index)
{
    if (index > size * 8) return false;
    uint64_t bytei = index / 8;
    uint8_t biti = index % 8;
    uint8_t bitindexer = 0b10000000 >> biti;
    if ((buffer[bytei] & bitindexer) > 0) return true;
    return false;
}

bool Bitmap::Set(uint64_t index, bool value)
{
    if (index > size * 8) return false;
    uint64_t bytei = index / 8;
    uint8_t biti = index % 8;
    uint8_t bitindexer = 0b10000000 >> biti;
    buffer[bytei] &= ~bitindexer;
    if (value)
    {
        buffer[bytei] |= bitindexer;
    }
    return true;
}