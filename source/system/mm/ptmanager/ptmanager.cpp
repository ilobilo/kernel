#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/mm/pmindexer/pmindexer.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/memory/memory.hpp>
#include <lib/string.hpp>

PTManager globalPTManager = NULL;

PTManager::PTManager(PTable *PML4Address)
{
    this->PML4 = PML4Address;
}

void PTManager::mapMem(void *virtualMemory, void *physicalMemory)
{
    PMIndexer indexer = PMIndexer((uint64_t)virtualMemory);
    PDEntry PDE;

    PDE = PML4->entries[indexer.PDP_i];
    PTable *PDP;
    if (!PDE.getflag(PT_Flag::Present))
    {
        PDP = (PTable*)globalAlloc.requestPage();
        memset(PDP, 0, 4096);
        PDE.setAddr((uint64_t)PDP >> 12);
        PDE.setflag(PT_Flag::Present, true);
        PDE.setflag(PT_Flag::ReadWrite, true);
        PML4->entries[indexer.PDP_i] = PDE;
    }
    else
    {
        PDP = (PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PDP->entries[indexer.PD_i];
    PTable *PD;
    if (!PDE.getflag(PT_Flag::Present))
    {
        PD = (PTable*)globalAlloc.requestPage();
        memset(PD, 0, 4096);
        PDE.setAddr((uint64_t)PD >> 12);
        PDE.setflag(PT_Flag::Present, true);
        PDE.setflag(PT_Flag::ReadWrite, true);
        PDP->entries[indexer.PD_i] = PDE;
    }
    else
    {
        PD = (PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PD->entries[indexer.PT_i];
    PTable *PT;
    if (!PDE.getflag(PT_Flag::Present))
    {
        PT = (PTable*)globalAlloc.requestPage();
        memset(PT, 0, 4096);
        PDE.setAddr((uint64_t)PT >> 12);
        PDE.setflag(PT_Flag::Present, true);
        PDE.setflag(PT_Flag::ReadWrite, true);
        PD->entries[indexer.PT_i] = PDE;
    }
    else
    {
        PT = (PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PT->entries[indexer.P_i];
    PDE.setAddr((uint64_t)physicalMemory >> 12);
    PDE.setflag(PT_Flag::Present, true);
        PDE.setflag(PT_Flag::ReadWrite, true);
    PT->entries[indexer.P_i] = PDE;
}

CRs getCRs()
{
    uint64_t cr0, cr2, cr3;
    __asm__ __volatile__ (
        "mov %%cr0, %%rax\n\t"
        "mov %%eax, %0\n\t"
        "mov %%cr2, %%rax\n\t"
        "mov %%eax, %1\n\t"
        "mov %%cr3, %%rax\n\t"
        "mov %%eax, %2\n\t"
    : "=m" (cr0), "=m" (cr2), "=m" (cr3)
    : /* no input */
    : "%rax"
    );
    return {cr0, cr2, cr3};
}

extern "C" uint64_t __kernelstart;
extern "C" uint64_t __kernelend;
void PTManager_init()
{
    serial_info("Initialising Page Table Manager\n");

    uint64_t kernelsize = (uint64_t)&__kernelend - (uint64_t)&__kernelstart;
    uint64_t kernelpagecount = (uint64_t)kernelsize / 4096 + 1;

    globalAlloc.lockPages((void*)&__kernelstart, kernelpagecount);

    PTable *PML4 = (PTable*)getCRs().cr3;

    globalPTManager = PTManager(PML4);

    for (int s = 0; s < getmemsize(); s += 0x1000)
    {
        globalPTManager.mapMem((void *)(s + 0xFFFF800000000000), (void *)s);
    }
}