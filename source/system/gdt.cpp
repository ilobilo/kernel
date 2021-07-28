#include "gdt.hpp"

__attribute__((aligned(0x1000)))
GDT DefaultGDT = {
    // bit 53 (L) must be set on code descriptors,
    // in those, bit 54 (Sz) must be 0.
    {0, 0, 0, 0x00, 0x00, 0}, // NULL
    {0xffff, 0, 0, 0x9a, 0x80, 0}, // 16bit_code
    {0xffff, 0, 0, 0x92, 0x80, 0}, // 16bit_data
    {0xffff, 0, 0, 0x9a, 0xcf, 0}, // 32bit_code
    {0xffff, 0, 0, 0x92, 0xcf, 0}, // 32bit_data
    {0, 0, 0, 0x9a, 0xa2, 0}, // 64bit_code
    {0, 0, 0, 0x92, 0xa0, 0}, // 64bit_data
};

bool GDT_init()
{
    GDTDescriptor gdtDescriptor;
    gdtDescriptor.Size = sizeof(GDT) - 1;
    gdtDescriptor.Offset = (uint64_t)&DefaultGDT;
    LoadGDT(&gdtDescriptor);
    return true;
}
