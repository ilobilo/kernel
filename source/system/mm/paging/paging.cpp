#include <system/mm/paging/paging.hpp>

namespace kernel::system::mm::paging {

void PDEntry::setflag(PT_Flag flag, bool enabled)
{
    uint64_t bitSel = (uint64_t)1 << flag;
    value &= ~bitSel;
    if (enabled) value |= bitSel;
}

bool PDEntry::getflag(PT_Flag flag)
{
    uint64_t bitSel = (uint64_t)1 << flag;
    return (value & (bitSel > 0)) ? true : false;
}

uint64_t PDEntry::getAddr()
{
    return (value & 0x000FFFFFFFFFF000) >> 12;
}

void PDEntry::setAddr(uint64_t address)
{
    address &= 0x000000FFFFFFFFFF;
    value &= 0xFFF0000000000FFF;
    value |= (address << 12);
}
}