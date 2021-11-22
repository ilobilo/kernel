// Copyright (C) 2021  ilobilo

#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/mm/pfalloc/pfalloc.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/mm/heap/heap.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;
using namespace kernel::drivers;
using namespace kernel::system::sched;
using namespace kernel::system::mm;
using namespace kernel::system;

vfs::fs_node_t *current_path;

namespace kernel::apps::kshell {

void shell_parse(char *cmd, char *arg)
{
    switch (hash(cmd))
    {
        case hash("help"):
            printf("- help -- This\n");
            printf("- clear -- Clear terminal\n");
            printf("- ls -- List files\n");
            printf("- cat -- Print file contents\n");
            printf("- cd -- Change directory\n");
            printf("- exec -- Execute binary\n");
            printf("- free -- Get memory info in bytes\n");
            printf("-  -h -- Get memory info in MB\n");
            printf("- time -- Get current RTC time\n");
            printf("- timef -- Get current RTC time (Forever loop)\n");
            printf("- tick -- Get current PIT tick\n");
            printf("- pci -- List PCI devices\n");
            printf("- crash -- Crash whole system\n");
            printf("- reboot -- Reboot the system\n");
            printf("- poweroff -- Shutdown the system\n");
            printf("- shutdown -- Shutdown the system\n");
            break;
        case hash("clear"):
            terminal::clear();
            break;
        case hash("ls"):
        {
            vfs::fs_node_t *node;
            if (!strncmp(arg, "../", 3) || !strncmp(arg, "..", 2)) node = current_path->parent;
            else if (!strncmp(arg, "./", 2) || !strncmp(arg, ".", 1) || !strcmp(arg, "")) node = current_path;
            else if (!strncmp(arg, "/", 1)) node = vfs::open(0, arg);
            else node = vfs::open(current_path, arg);
            if (!node)
            {
                printf("\033[31mNo such file directory!%s\n", terminal::colour);
                return;
            }
            if ((node->flags & 0x07) != vfs::FS_DIRECTORY)
            {
                printf("%s\n", arg);
                break;
            }

            size_t size = 0;
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children.at(i)->flags & 0x07)
                {
                    case vfs::FS_DIRECTORY:
                        printf("\033[35m%s%s ", node->children.at(i)->name, terminal::colour);
                        break;
                }
            }
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children.at(i)->flags & 0x07)
                {
                    case vfs::FS_CHARDEVICE:
                        printf("\033[93m%s%s ", node->children.at(i)->name, terminal::colour);
                        break;
                }
            }
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children.at(i)->flags & 0x07)
                {
                    case vfs::FS_SYMLINK:
                        printf("\033[96m%s%s ", node->children.at(i)->name, terminal::colour);
                        break;
                }
            }
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children.at(i)->flags & 0x07)
                {
                    case vfs::FS_FILE:
                        printf("%s ", node->children.at(i)->name);
                        size += node->length;
                        break;
                }
            }
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children.at(i)->flags & 0x07)
                {
                    case vfs::FS_FILE:
                    case vfs::FS_SYMLINK:
                    case vfs::FS_DIRECTORY:
                    case vfs::FS_CHARDEVICE:
                        break;
                    default:
                        printf("\033[31m%s%s ", node->children.at(i)->name, terminal::colour);
                        break;
                }
            }
            printf("\n");
            break;
        }
        case hash("cat"):
        {
            vfs::fs_node_t *node;
            if (!strncmp(arg, "/", 1)) node = vfs::open(NULL, arg);
            else node = vfs::open(current_path, arg);
            if (!node)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                return;
            }
            switch (node->flags & 0x07)
            {
                case vfs::FS_FILE:
                case vfs::FS_CHARDEVICE:
                {
                    size_t size = 50;
                    if (node->length) size = node->length;
                    char *txt;
                    if (size <= 50)
                    {
                        txt = (char*)heap::calloc(node->length, sizeof(char));
                        vfs::read_fs(node, 0, size, txt);
                        printf("%s", txt);
                    }
                    else
                    {
                        size_t offset = 0;
                        txt = (char*)heap::calloc(50, sizeof(char));
                        while (offset < size - (size % 50))
                        {
                            memset(txt, 0, 50);
                            vfs::read_fs(node, offset, 50, txt);
                            printf("%s", txt);
                            offset += 50;
                        }
                        vfs::read_fs(node, offset, size % 50, txt);
                        printf("%s", txt);
                    }
                    printf("\n");
                    heap::free(txt);
                    break;
                }
                default:
                    printf("\033[31m%s is not a text file!%s\n", arg, terminal::colour);
                    break;
            }
            break;
        }
        case hash("cd"):
        {
            if (!strcmp(arg, ""))
            {
                current_path = vfs::getchild(0, "/");
                break;
            }
            if (!strcmp(arg, "..") || !strcmp(arg, "../"))
            {
                current_path = current_path->parent;
                break;
            }
            if (!strcmp(arg, ".") || !strcmp(arg, "./")) break;
            if (!strcmp(arg, "/"))
            {
                current_path = vfs::fs_root->ptr;
                break;
            }
            vfs::fs_node_t *node;
            if (!strncmp(arg, "/", 1)) node = vfs::open(0, arg);
            else node = vfs::open(current_path, arg);
            if (!node)
            {
                printf("\033[31mNo such directory!%s\n", terminal::colour);
                break;
            }
            if ((node->flags & 0x07) != vfs::FS_DIRECTORY)
            {
                printf("\033[31m%s is not a directory!%s\n", arg, terminal::colour);
                break;
            }
            current_path = node;
            break;
        }
        case hash("exec"):
        {
            if (!strcmp(arg, ""))
            {
                printf("exec <filename>\n");
                break;
            }
            vfs::fs_node_t *node = vfs::open(current_path, arg);
            if (!node)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                return;
            }
            if ((node->flags & 0x07) != vfs::FS_FILE)
            {
                printf("\033[31m%s is not an executable file!%s\n", arg, terminal::colour);
                break;
            }
            using func = int (*)();
            func t = (func)node->address;
            t();
            break;
        }
        case hash("free"):
        {
            double usable = getmemsize();
            double free = pfalloc::getFreeRam();
            if (!strcmp(arg, "-h"))
            {
                usable = usable / 1024 / 1024;
                free = free / 1024 / 1024;
                printf("Usable memory: %.2f MB\nFree memory: %.2f MB\nUsed memory: %.2f MB\n", usable, free, usable - free);
            }
            else printf("Usable memory: %.0f Bytes\nFree memory: %.0f Bytes\nUsed memory: %.0f Bytes\n", usable, free, usable - free);
            break;
        }
        case hash("time"):
            printf("%s\n", rtc::getTime());
            break;
        case hash("tick"):
            printf("%ld\n", pit::get_tick());
            break;
        case hash("timef"):
            while (true)
            {
                printf("\r\033[2K%s", rtc::getTime());
                pit::sleep(1);
            }
            break;
        case hash("pci"):
            for (size_t i = 0; i < pci::pcidevices.size(); i++)
            {
                printf("%.4X:%.4X %s %s\n",
                    pci::pcidevices[i]->vendorid,
                    pci::pcidevices[i]->deviceid,
                    pci::pcidevices[i]->vendorstr,
                    pci::pcidevices[i]->devicestr);
            }
            break;
        case hash("crash"):
            ((int (*)())vfs::open(NULL, "/bin/crash")->address)();
            break;
        case hash("shutdown"):
        case hash("poweroff"):
            outw(0xB004, 0x2000);
            outw(0x604, 0x2000);
            outw(0x4004, 0x3400);
            asm volatile ("hlt");
            break;
        case hash("reboot"):
            outb(acpi::fadthdr->ResetReg.Address, acpi::fadthdr->ResetValue);
            break;
        case hash(""):
            break;
        default:
            printf("\033[31mCommand not found!\033[0m\n");
            break;
    }
}

void run()
{
    if (!current_path)
    {
        current_path = vfs::getchild(NULL, "/");
        current_path->flags = vfs::FS_DIRECTORY;
    }
    printf("\033[32mroot@kernel\033[0m:\033[95m%s%s%s# ", (current_path->name[0] != '/') ? "/" : "", current_path->name, terminal::colour);
    char *command = ps2::kbd::getline();
    char cmd[10] = "\0";

    for (size_t i = 0; i < strlen(command); i++)
    {
        if (command[i] != ' ' && command[i] != '\0')
        {
            char c[2] = "\0";
            c[0] = command[i];
            strcat(cmd, c);
        }
        else break;
    }
    char *arg = strrm(command, cmd);
    arg = strrm(arg, " ");

    shell_parse(cmd, arg);
}
}