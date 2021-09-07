#include <drivers/terminal/terminal.hpp>
#include <drivers/keyboard/keyboard.hpp>
#include <drivers/fs/tar/tar.hpp>
#include <include/string.hpp>

void shell_parse(char* cmd)
{
    if(!strcmp(cmd, "ls"))
    {
        tar_list();
    }
}

void shell_run()
{
    printf("root@kernel:~# ");
    char* command = getline();
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

    shell_parse(cmd);
}