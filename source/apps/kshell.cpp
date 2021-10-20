#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <system/mm/memory/memory.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>

void shell_parse(char *cmd, char *arg)
{
    if (!strcmp(cmd, "help"))
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
    else if (!strcmp(cmd, "clear")) term_clear();
    else if (!strcmp(cmd, "ls")) ustar_list();
    else if (!strcmp(cmd, "cat")) ustar_cat(arg);
    else if (!strcmp(cmd, "free"))
    {
        double usable = getmemsize();
        double free = globalAlloc.getFreeRam();
        if (!strcmp(arg, "-h"))
        {
            usable = usable / 1024 / 1024;
            free = free / 1024 / 1024;
            printf("Usable memory: %.2f MB\nFree memory: %.2f MB\nUsed memory: %.2f MB\n", usable, free, usable - free);
        }
        else printf("Usable memory: %.0f Bytes\nFree memory: %.0f Bytes\nUsed memory: %.0f Bytes\n", usable, free, usable - free);
    }
    else if (!strcmp(cmd, "time")) printf("%s\n", RTC_GetTime());
    else if (!strcmp(cmd, "tick")) printf("%ld\n", get_tick());
    else if (!strcmp(cmd, "timef"))
    {
        while (true)
        {
            printf("%s", RTC_GetTime());
            PIT_sleep(1);
            printf("\r\033[2K");
        }
    }
    else if (!strcmp(cmd, "pci"))
    {
        for (uint64_t i = 0; i < pcidevcount; i++)
        {
            printf("%s / %s / %s / %s / %s\n",
                pcidevices[i]->vendorstr,
                pcidevices[i]->devicestr,
                pcidevices[i]->ClassStr,
                pcidevices[i]->subclassStr,
                pcidevices[i]->progifstr);
        }
    }
    else if (!strcmp(cmd, "crash"))
    {
        asm volatile ("int $0x3");
        asm volatile ("int $0x4");
    }
    else if (strcmp(cmd, "")) printf("\033[31mCommand not found!\033[0m\n");
}

void shell_run()
{
    printf("root@kernel:~# ");
    char *command = getline();
    char *arg = "\0";
    char *cmd = "\0";

    for (size_t i = 0; i < strlen(cmd); i++)
    {
        cmd[i] = '\0';
    }
    arg = command;

    // Get cmd string
    for (size_t i = 0; i < strlen(command); i++)
    {
        if (command[i] != ' ' && command[i] != '\0')
        {
            char c[2] = "\0";
            c[0] = command[i];
            strcat(cmd, c);
        }
        else
        {
            break;
        }
    }
    // Remove cmd from arg
    if (strlen(cmd) != strlen(command))
    {
        for (size_t i = 0; i < strlen(cmd) + 1; i++)
        {
            arg++;
        }
    }

    shell_parse(cmd, arg);
}