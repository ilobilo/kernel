#include <drivers/display/serial/serial.hpp>
#include <system/mm/ptmanager/ptmanager.hpp>
#include <system/mm/pmindexer/pmindexer.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>

using namespace kernel::drivers::display;
using namespace kernel::lib;

namespace kernel::system::mm::ptmanager {

PTManager globalPTManager = NULL;
bool initialised = false;

PTManager::PTManager(paging::PTable *PML4Address)
{
    this->PML4 = PML4Address;
}

void PTManager::mapMem(void *virtualMemory, void *physicalMemory)
{
    pmindexer::PMIndexer indexer = pmindexer::PMIndexer((uint64_t)virtualMemory);
    paging::PDEntry PDE;

    PDE = PML4->entries[indexer.PDP_i];
    paging::PTable *PDP;
    if (!PDE.getflag(paging::PT_Flag::Present))
    {
        PDP = (paging::PTable*)pfalloc::requestPage();
        memory::memset(PDP, 0, 4096);
        PDE.setAddr((uint64_t)PDP >> 12);
        PDE.setflag(paging::PT_Flag::Present, true);
        PDE.setflag(paging::PT_Flag::ReadWrite, true);
        PML4->entries[indexer.PDP_i] = PDE;
    }
    else
    {
        PDP = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PDP->entries[indexer.PD_i];
    paging::PTable *PD;
    if (!PDE.getflag(paging::PT_Flag::Present))
    {
        PD = (paging::PTable*)pfalloc::requestPage();
        memory::memset(PD, 0, 4096);
        PDE.setAddr((uint64_t)PD >> 12);
        PDE.setflag(paging::PT_Flag::Present, true);
        PDE.setflag(paging::PT_Flag::ReadWrite, true);
        PDP->entries[indexer.PD_i] = PDE;
    }
    else
    {
        PD = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PD->entries[indexer.PT_i];
    paging::PTable *PT;
    if (!PDE.getflag(paging::PT_Flag::Present))
    {
        PT = (paging::PTable*)pfalloc::requestPage();
        memory::memset(PT, 0, 4096);
        PDE.setAddr((uint64_t)PT >> 12);
        PDE.setflag(paging::PT_Flag::Present, true);
        PDE.setflag(paging::PT_Flag::ReadWrite, true);
        PD->entries[indexer.PT_i] = PDE;
    }
    else
    {
        PT = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PT->entries[indexer.P_i];
    PDE.setAddr((uint64_t)physicalMemory >> 12);
    PDE.setflag(paging::PT_Flag::Present, true);
    PDE.setflag(paging::PT_Flag::ReadWrite, true);
    PT->entries[indexer.P_i] = PDE;
}

void PTManager::unmapMem(void *virtualMemory)
{
    pmindexer::PMIndexer indexer = pmindexer::PMIndexer((uint64_t)virtualMemory);
    paging::PDEntry PDE;

    PDE = PML4->entries[indexer.PDP_i];
    paging::PTable *PDP;
    if (!PDE.getflag(paging::PT_Flag::Present))
    {
        PDP = (paging::PTable*)pfalloc::requestPage();
        memory::memset(PDP, 0, 4096);
        PDE.setAddr((uint64_t)PDP >> 12);
        PDE.setflag(paging::PT_Flag::Present, true);
        PDE.setflag(paging::PT_Flag::ReadWrite, true);
        PML4->entries[indexer.PDP_i] = PDE;
    }
    else
    {
        PDP = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PDP->entries[indexer.PD_i];
    paging::PTable *PD;
    if (!PDE.getflag(paging::PT_Flag::Present))
    {
        PD = (paging::PTable*)pfalloc::requestPage();
        memory::memset(PD, 0, 4096);
        PDE.setAddr((uint64_t)PD >> 12);
        PDE.setflag(paging::PT_Flag::Present, true);
        PDE.setflag(paging::PT_Flag::ReadWrite, true);
        PDP->entries[indexer.PD_i] = PDE;
    }
    else
    {
        PD = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PD->entries[indexer.PT_i];
    paging::PTable *PT;
    if (!PDE.getflag(paging::PT_Flag::Present))
    {
        PT = (paging::PTable*)pfalloc::requestPage();
        memory::memset(PT, 0, 4096);
        PDE.setAddr((uint64_t)PT >> 12);
        PDE.setflag(paging::PT_Flag::Present, true);
        PDE.setflag(paging::PT_Flag::ReadWrite, true);
        PD->entries[indexer.PT_i] = PDE;
    }
    else
    {
        PT = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    }

    PDE = PT->entries[indexer.P_i];
    PDE.setAddr(0);
    PDE.setflag(paging::PT_Flag::Present, false);
    PDE.setflag(paging::PT_Flag::ReadWrite, false);
    PT->entries[indexer.P_i] = PDE;
}

void PTManager::mapUserMem(void *virtualMemory)
{
    pmindexer::PMIndexer indexer = pmindexer::PMIndexer((uint64_t)virtualMemory);
    paging::PDEntry PDE;

    PDE = PML4->entries[indexer.PDP_i];
    PDE.setflag(paging::PT_Flag::UserSuper, true);
    PML4->entries[indexer.PDP_i] = PDE;

    paging::PTable *L3 = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    PDE = L3->entries[indexer.PD_i];
    PDE.setflag(paging::PT_Flag::UserSuper, true);
    L3->entries[indexer.PD_i] = PDE;

    paging::PTable *L2 = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    PDE = L2->entries[indexer.PT_i];
    PDE.setflag(paging::PT_Flag::UserSuper, true);
    L2->entries[indexer.PT_i] = PDE;

    paging::PTable *L1 = (paging::PTable*)((uint64_t)PDE.getAddr() << 12);
    PDE = L1->entries[indexer.P_i];
    PDE.setflag(paging::PT_Flag::UserSuper, true);
    L1->entries[indexer.P_i] = PDE;
}

CRs getCRs()
{
    uint64_t cr0, cr2, cr3;
    asm volatile (
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
void init()
{
    serial::info("Initialising Page Table Manager\n");

    if (initialised)
    {
        serial::info("Page table manager has already been initialised!\n");
        return;
    }

    if (!pfalloc::initialised)
    {
        serial::info("Page frame allocator has not been initialised!\n");
        pfalloc::init();
    }

    uint64_t kernelsize = (uint64_t)&__kernelend - (uint64_t)&__kernelstart;
    uint64_t kernelpagecount = (uint64_t)kernelsize / 4096 + 1;

    pfalloc::lockPages((void*)&__kernelstart, kernelpagecount);

    paging::PTable *PML4 = (paging::PTable*)getCRs().cr3;
    globalPTManager = PTManager(PML4);

    initialised = true;
}
}