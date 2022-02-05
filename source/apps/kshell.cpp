// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <system/sched/timer/timer.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/acpi/acpi.hpp>
#include <drivers/ps2/ps2.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/alloc.hpp>
#include <lib/cwalk.hpp>
#include <lib/log.hpp>
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
            printf("- tick -- Get current PIT tick\n");
            printf("- pci -- List PCI devices\n");
            printf("- crash -- Crash whole system\n");
            printf("- reboot -- Reboot the system\n");
            printf("- poweroff -- Shutdown the system\n");
            printf("- shutdown -- Shutdown the system\n");
            printf("You can execute any file in /bin with just typing it's name or with \"exec <filename>\"\n");
            break;
        case hash("clear"):
            terminal::clear();
            break;
        case hash("ls"):
        {
            char *path = new char[strlen(arg) + 1];
            cwk_path_normalize(arg, path, strlen(arg) + 1);

            vfs::fs_node_t *node = vfs::open(current_path, path);
            if (!node)
            {
                printf("\033[31mNo such file directory!%s\n", terminal::colour);
                delete[] path;
                break;
            }
            if ((node->flags & 0x07) != vfs::FS_DIRECTORY)
            {
                printf("%s\n", path);
                delete[] path;
                break;
            }
            delete[] path;

            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children[i]->flags & 0x07)
                {
                    case vfs::FS_DIRECTORY:
                        printf("\033[35m%s%s ", node->children[i]->name, terminal::colour);
                        break;
                }
            }
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children[i]->flags & 0x07)
                {
                    case vfs::FS_CHARDEVICE:
                        printf("\033[93m%s%s ", node->children[i]->name, terminal::colour);
                        break;
                }
            }
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children[i]->flags & 0x07)
                {
                    case vfs::FS_SYMLINK:
                        printf("\033[96m%s%s ", node->children[i]->name, terminal::colour);
                        break;
                }
            }
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children[i]->flags & 0x07)
                {
                    case vfs::FS_FILE:
                        printf("%s ", node->children[i]->name);
                        break;
                }
            }
            for (size_t i = 0; i < node->children.size(); i++)
            {
                switch (node->children[i]->flags & 0x07)
                {
                    case vfs::FS_FILE:
                    case vfs::FS_SYMLINK:
                    case vfs::FS_DIRECTORY:
                    case vfs::FS_CHARDEVICE:
                        break;
                    default:
                        printf("\033[31m%s%s ", node->children[i]->name, terminal::colour);
                        break;
                }
            }
            printf("\n");
            break;
        }
        case hash("cat"):
        {
            char *path = new char[strlen(arg) + 1];
            cwk_path_normalize(arg, path, strlen(arg) + 1);

            vfs::fs_node_t *node = vfs::open(current_path, path);
            if (!node)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                delete[] path;
                break;
            }
            switch (node->flags & 0x07)
            {
                case vfs::FS_FILE:
                    printf("%.*s%c", static_cast<int>(node->length), reinterpret_cast<char*>(node->address), (reinterpret_cast<char*>(node->address)[node->length - 1] != '\n') ? '\n' : 0);
                    break;
                case vfs::FS_CHARDEVICE:
                {
                    size_t size = 50;
                    if (node->length) size = node->length;
                    char *txt = static_cast<char*>(calloc(size, sizeof(char)));
                    vfs::read_fs(node, 0, size, txt);
                    printf("%.*s\n", static_cast<int>(size), txt);
                    free(txt);
                    break;
                }
                default:
                    printf("\033[31m%s is not a text file!%s\n", path, terminal::colour);
                    break;
            }
            delete[] path;
            break;
        }
        case hash("cd"):
        {
            if (isempty(arg)) strcpy(arg, "/");

            char *path = new char[strlen(arg) + 1];
            cwk_path_normalize(arg, path, strlen(arg) + 1);

            vfs::fs_node_t *node = vfs::open(current_path, path);
            if (!node)
            {
                printf("\033[31mNo such directory!%s\n", terminal::colour);
                delete[] path;
                break;
            }
            if ((node->flags & 0x07) != vfs::FS_DIRECTORY)
            {
                printf("\033[31m%s is not a directory!%s\n", path, terminal::colour);
                delete[] path;
                break;
            }
            current_path = node;
            delete[] path;
            break;
        }
        case hash("exec"):
        {
            if (isempty(arg))
            {
                printf("exec <filename>\n");
                break;
            }
            char *path = new char[strlen(arg) + 1];
            cwk_path_normalize(arg, path, strlen(arg) + 1);

            vfs::fs_node_t *node = vfs::open(current_path, path);
            if (!node)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                delete[] path;
                break;
            }
            if ((node->flags & 0x07) != vfs::FS_FILE)
            {
                printf("\033[31m%s is not an executable file!%s\n", path, terminal::colour);
                delete[] path;
                break;
            }
            reinterpret_cast<int (*)()>(node->address)();
            delete[] path;
            break;
        }
        case hash("free"):
        {
            uint64_t free = pmm::freemem() / 1024;
            uint64_t used = pmm::usedmem() / 1024;
            uint64_t all = free + used;
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
                timer::sleep(1);
            }
            break;
        case hash("pci"):
            for (size_t i = 0; i < pci::devices.size(); i++)
            {
                printf("%.4X:%.4X %s %s\n",
                    pci::devices[i]->device->vendorid,
                    pci::devices[i]->device->deviceid,
                    pci::devices[i]->vendorstr,
                    pci::devices[i]->devicestr);
            }
            break;
        case hash("crash"):
            reinterpret_cast<int (*)()>(vfs::open(nullptr, "/bin/crash")->address)();
            break;
        case hash("shutdown"):
        case hash("poweroff"):
            acpi::shutdown();
            timer::sleep(1);
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
        {
            vfs::fs_node_t *node = vfs::open(vfs::open(nullptr, "/bin"), cmd);
            if (node != nullptr)
            {
                if ((node->flags & 0x07) != vfs::FS_FILE)
                {
                    printf("\033[31m%s is not an executable file!%s\n", cmd, terminal::colour);
                    break;
                }
                reinterpret_cast<int (*)()>(node->address)();
            }
            else printf("\033[31mCommand not found!\033[0m\n");
            break;
        }
    }
}

void run()
{
    log("Starting kernel shell\n");
    while (true)
    {
        if (!current_path)
        {
            current_path = scheduler::this_proc()->current_dir;
            current_path->flags = vfs::FS_DIRECTORY;
        }
        printf("\033[32mroot@kernel\033[0m:\033[95m%s%s%s# ", (current_path->name[0] != '/') ? "/" : "", current_path->name, terminal::colour);
        char *command = ps2::getline();
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