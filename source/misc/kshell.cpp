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
    char* command = getline();;

    shell_parse(command);
}