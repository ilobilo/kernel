#pragma once

#include <system/mm/paging/paging.hpp>

namespace kernel::system::mm::ptmanager {

class PTManager
{
    public:
    PTManager(paging::PTable *PML4Address);
    paging::PTable *PML4;

    void mapMem(void *virtualMemory, void *physicalMemory);
    void unmapMem(void *virtualMemory);
    void mapUserMem(void *virtualMemory);
};

struct CRs
{
    uint64_t cr0;
    uint64_t cr2;
    uint64_t cr3;
};

extern PTManager globalPTManager;
extern bool initialised;

paging::PTable *clonePTable(paging::PTable *oldptable);
void switchPTable(paging::PTable *ptable);

void init();
}