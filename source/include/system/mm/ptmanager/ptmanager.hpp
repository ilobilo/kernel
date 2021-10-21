#pragma once

#include <system/mm/paging/paging.hpp>

class PTManager
{
    public:
    PTManager(PTable *PML4Address);
    PTable *PML4;
    void mapMem(void *virtualMemory, void *physicalMemory);
    void unmapMem(void *virtualMemory);
};

struct CRs
{
    uint64_t cr0;
    uint64_t cr2;
    uint64_t cr3;
};

extern PTManager globalPTManager;

extern bool ptmanager_initialised;

void PTManager_init();