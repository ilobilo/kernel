#include <drivers/devices/ps2/keyboard/keyboard.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <include/string.hpp>

void shell_parse(char* cmd, char* arg)
{
    if (!strcmp(cmd, "ls"))
    {
        initrd_list();
    }
    else if (!strcmp(cmd, "crash"))
    {
        asm volatile ("int $0x3");
        asm volatile ("int $0x4");
    }
    else if (strcmp(cmd, ""))
    {
        printf("\033[31mCommand not found!\033[0m\n");
    }
}

void shell_run()
{
    printf("root@kernel:~# ");
    char* command = getline();
    char* arg = command;
    char* cmd = "\0";

    for (int i = 0; i < strlen(cmd); i++)
    {
        cmd[i] = '\0';
    }

    // Get cmd string
    for (int i = 0; i < strlen(command); i++)
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
        for (int i = 0; i < strlen(cmd) + 1; i++)
        {
            arg++;
        }
    }

    shell_parse(cmd, arg);
}
