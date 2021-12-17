// Copyright (C) 2021  ilobilo

#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/acpi/acpi.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/buddy.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;
using namespace kernel::drivers;
using namespace kernel::system::sched;
using namespace kernel::system::mm;
using namespace kernel::system;

namespace kernel::apps::kshell {

vfs::fs_node_t *current_path;

void parse(char *cmd, char *arg)
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
            printf("- free -- Get memory info\n");
            printf("- time -- Get current RTC time\n");
            printf("- timef -- Get current RTC time (Forever loop)\n");
            printf("- fps -- Show FPS (Forever loop)\n");
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
            if (!strcmp(arg, "../") || !strcmp(arg, "..")) node = current_path->parent;
            else if (!strcmp(arg, "./") || !strcmp(arg, ".") || !strcmp(arg, "")) node = current_path;
            else if (!strncmp(arg, "/", 1)) node = vfs::open(nullptr, arg);
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
            if (!strncmp(arg, "/", 1)) node = vfs::open(nullptr, arg);
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
                    txt = static_cast<char*>(calloc(size, sizeof(char)));
                    vfs::read_fs(node, 0, size, txt);
                    printf("%.*s\n", static_cast<int>(size), txt);
                    free(txt);
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
                return;
            }
            if (!strcmp(arg, "..") || !strcmp(arg, "../"))
            {
                current_path = current_path->parent;
                return;
            }
            if (!strcmp(arg, ".") || !strcmp(arg, "./")) return;
            if (!strcmp(arg, "/"))
            {
                current_path = vfs::fs_root->ptr;
                return;
            }
            vfs::fs_node_t *node;
            if (!strncmp(arg, "/", 1)) node = vfs::open(nullptr, arg);
            else node = vfs::open(current_path, arg);
            if (!node)
            {
                printf("\033[31mNo such directory!%s\n", terminal::colour);
                return;
            }
            if ((node->flags & 0x07) != vfs::FS_DIRECTORY)
            {
                printf("\033[31m%s is not a directory!%s\n", arg, terminal::colour);
                return;
            }
            current_path = node;
            break;
        }
        case hash("exec"):
        {
            if (!strcmp(arg, ""))
            {
                printf("exec <filename>\n");
                return;
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
                return;
            }
            reinterpret_cast<int (*)()>(node->address)();
            break;
        }
        case hash("free"):
        {
            uint64_t all = getmemsize() / 1024;
            uint64_t free = pmm::freemem() / 1024;
            uint64_t used = pmm::usedmem() / 1024;
            printf("Usable memory: %ld KB\nFree memory: %ld KB\nUsed memory: %ld KB\n", all, free, used);
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
        case hash("fps"):
        {
            int time = 0, last_time = 0, fps = 0, frames = 0;
            while (true)
            {
                frames++;
                time = rtc::second();
                if (time != last_time)
                {
                    fps = frames;
                    frames = 0;
                    last_time = time;
                    printf("\r\033[2KFPS: %d", fps);
                }
            }
        }
        case hash("pci"):
            for (size_t i = 0; i < pci::pcidevices.size(); i++)
            {
                printf("%.4X:%.4X %s %s\n",
                    pci::pcidevices[i]->device->vendorid,
                    pci::pcidevices[i]->device->deviceid,
                    pci::pcidevices[i]->vendorstr,
                    pci::pcidevices[i]->devicestr);
            }
            break;
        case hash("crash"):
            reinterpret_cast<int (*)()>(vfs::open(nullptr, "/bin/crash")->address)();
            break;
        case hash("shutdown"):
        case hash("poweroff"):
            acpi::shutdown();
            pit::msleep(50);
            outw(0xB004, 0x2000);
            outw(0x604, 0x2000);
            outw(0x4004, 0x3400);
            printf("\033[31mCould not shutdown!\033[0m\n");
            break;
        case hash("reboot"):
            acpi::reboot();
            printf("\033[31mCould not reboot!\033[0m\n");
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
    serial::info("Starting kernel shell\n");
    while (true)
    {
        if (!current_path)
        {
            current_path = scheduler::running_thread()->current_dir;
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

        parse(cmd, arg);
    }
}
}