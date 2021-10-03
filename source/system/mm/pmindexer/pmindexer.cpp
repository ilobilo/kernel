#include <system/mm/pmindexer/pmindexer.hpp>

PMIndexer::PMIndexer(uint64_t virtualAddress)
{
    virtualAddress >>= 12;
    P_i = virtualAddress & 0x1FF;
    virtualAddress >>= 9;
    PT_i = virtualAddress & 0x1FF;
    virtualAddress >>= 9;
    PD_i = virtualAddress & 0x1FF;
    virtualAddress >>= 9;
    PDP_i = virtualAddress & 0x1FF;
}