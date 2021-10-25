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
using namespace kernel::system::mm;
using namespace kernel::system;
using namespace kernel::lib;

namespace kernel::apps::kshell {

void shell_parse(char *cmd, char *arg)
{
    switch (string::hash(cmd))
    {
        case string::hash("help"):
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
            break;
        case string::hash("clear"):
            terminal::clear();
            break;
        case string::hash("ls"):
            ustar::list();
            break;
        case string::hash("cat"):
            ustar::cat(arg);
            break;
        case string::hash("free"):
        {
            double usable = memory::getmemsize();
            double free = pfalloc::getFreeRam();
            if (!string::strcmp(arg, "-h"))
            {
                usable = usable / 1024 / 1024;
                free = free / 1024 / 1024;
                printf("Usable memory: %.2f MB\nFree memory: %.2f MB\nUsed memory: %.2f MB\n", usable, free, usable - free);
            }
            else printf("Usable memory: %.0f Bytes\nFree memory: %.0f Bytes\nUsed memory: %.0f Bytes\n", usable, free, usable - free);
            break;
        }
        case string::hash("time"):
            printf("%s\n", rtc::getTime());
            break;
        case string::hash("tick"):
            printf("%ld\n", pit::get_tick());
            break;
        case string::hash("timef"):
            while (true)
            {
                printf("%s", rtc::getTime());
                pit::sleep(1);
                printf("\r\033[2K");
            }
            break;
        case string::hash("pci"):
            for (uint64_t i = 0; i < pci::pcidevcount; i++)
            {
                printf("%s / %s / %s / %s / %s\n",
                    pci::pcidevices[i]->vendorstr,
                    pci::pcidevices[i]->devicestr,
                    pci::pcidevices[i]->ClassStr,
                    pci::pcidevices[i]->subclassStr,
                    pci::pcidevices[i]->progifstr);
            }
            break;
        case string::hash("crash"):
            asm volatile ("int $0x3");
            asm volatile ("int $0x4");
            break;
        case string::hash(""):
            break;
        default:
            printf("\033[31mCommand not found!\033[0m\n");
            break;
    }
}

void run()
{
    printf("root@kernel:~# ");
    char *command = ps2::kbd::getline();
    char cmd[10] = "\0";

    for (size_t i = 0; i < string::strlen(command); i++)
    {
        if (command[i] != ' ' && command[i] != '\0')
        {
            char c[2] = "\0";
            c[0] = command[i];
            string::strcat(cmd, c);
        }
        else break;
    }
    char *arg = string::strrm(command, cmd);
    arg = string::strrm(arg, " ");

    shell_parse(cmd, arg);
}
}