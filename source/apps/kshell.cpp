#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;
using namespace kernel::drivers;
using namespace kernel::system::sched;
using namespace kernel::system;
using namespace kernel::lib;

namespace kernel::apps::kshell {

void shell_parse(char *cmd, char *arg)
{
    if (!string::strcmp(cmd, "help"))
    {
        printf("Supported commands:\n");
        printf("- help\t-- This\n");
        printf("- clear\t-- Clear terminal\n");
        printf("- ls\t-- List files\n");
        printf("- free\t-- Get memory info in bytes\n");
        printf("-  -h\t-- Get memory info in MB\n");
        printf("- time\t-- Get current RTC time\n");
        printf("- timef\t-- Get current RTC time (Forever loop)\n");
        printf("- tick\t-- Get current PIT tick\n");
        printf("- pci\t-- List PCI devices\n");
        printf("- crash\t-- Crash whole system\n");
    }
    else if (!string::strcmp(cmd, "clear")) terminal::clear();
    else if (!string::strcmp(cmd, "ls")) ustar::list();
    else if (!string::strcmp(cmd, "cat")) ustar::cat(arg);
    else if (!string::strcmp(cmd, "free"))
    {
        double usable = memory::getmemsize();
        double free = system::mm::pfalloc::globalAlloc.getFreeRam();
        if (!string::strcmp(arg, "-h"))
        {
            usable = usable / 1024 / 1024;
            free = free / 1024 / 1024;
            printf("Usable memory: %.2f MB\nFree memory: %.2f MB\nUsed memory: %.2f MB\n", usable, free, usable - free);
        }
        else printf("Usable memory: %.0f Bytes\nFree memory: %.0f Bytes\nUsed memory: %.0f Bytes\n", usable, free, usable - free);
    }
    else if (!string::strcmp(cmd, "time")) printf("%s\n", rtc::getTime());
    else if (!string::strcmp(cmd, "tick")) printf("%ld\n", pit::get_tick());
    else if (!string::strcmp(cmd, "timef"))
    {
        while (true)
        {
            printf("%s", rtc::getTime());
            pit::sleep(1);
            printf("\r\033[2K");
        }
    }
    else if (!string::strcmp(cmd, "pci"))
    {
        for (uint64_t i = 0; i < pci::pcidevcount; i++)
        {
            printf("%s / %s / %s / %s / %s\n",
                pci::pcidevices[i]->vendorstr,
                pci::pcidevices[i]->devicestr,
                pci::pcidevices[i]->ClassStr,
                pci::pcidevices[i]->subclassStr,
                pci::pcidevices[i]->progifstr);
        }
    }
    else if (!string::strcmp(cmd, "crash"))
    {
        asm volatile ("int $0x3");
        asm volatile ("int $0x4");
    }
    else if (string::strcmp(cmd, "")) printf("\033[31mCommand not found!\033[0m\n");
}

void run()
{
    printf("root@kernel:~# ");
    char *command = ps2::kbd::getline();
    char *arg = (char*)heap::calloc(10, sizeof(char));
    char *cmd = (char*)heap::calloc(10, sizeof(char));

    for (size_t i = 0; i < string::strlen(cmd); i++)
    {
        cmd[i] = '\0';
    }
    arg = command;

    // Get cmd string
    for (size_t i = 0; i < string::strlen(command); i++)
    {
        if (command[i] != ' ' && command[i] != '\0')
        {
            char c[2] = "\0";
            c[0] = command[i];
            string::strcat(cmd, c);
        }
        else
        {
            break;
        }
    }
    // Remove cmd from arg
    if (string::strlen(cmd) != string::strlen(command))
    {
        for (size_t i = 0; i < string::strlen(cmd) + 1; i++)
        {
            arg++;
        }
    }

    shell_parse(cmd, arg);
    heap::free(arg);
    heap::free(cmd);
}
}