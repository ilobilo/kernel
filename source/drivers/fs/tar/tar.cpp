#include <drivers/fs/tar/tar.hpp>
#include <drivers/serial/serial.hpp>
#include <drivers/terminal/terminal.hpp>
#include <include/string.hpp>

tar_header_t* tar_headers;

unsigned int getsize(const char *in)
{
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;
    for (j = 11; j > 0; j--, count *= 8)
    {
        size += ((in[j - 1] - '0') * count);
    }
    return size;
}

unsigned int initrd_parse(unsigned int address)
{
    unsigned int i;
    for (i = 0; ; i++)
    {
        struct tar_file_header_t* header = (tar_file_header_t *)address;

        if (header->name[0] == '\0')
        {
            break;
        }
        unsigned int size = getsize(header->size);
        tar_headers->headers[i] = header;
        tar_headers->address[i] = address + 512;
        tar_headers->count++;

        address += ((size / 512) + 1) * 512;

        if (size % 512)
        {
            address += 512;
        }
    }
    for (int g = 1; g < tar_headers->count; g++)
    {
        memmove(tar_headers->headers[g]->name, tar_headers->headers[g]->name + 1, strlen(tar_headers->headers[g]->name));
    }
    tar_headers->count--;
    i--;
    return i;
}

void initrd_list()
{
    int size = 0;
    printf("Total %d items:\n", tar_headers->count);
    for (int i = 1; i < tar_headers->count + 1; i++)
    {
        printf("%d) %s %dB\n", i, tar_headers->headers[i]->name, oct_to_dec(string_to_int(tar_headers->headers[i]->size)));
        size += oct_to_dec(string_to_int(tar_headers->headers[i]->size));
    }
    printf("Total size: %dB\n", size);
}

void initrd_cat(char* name)
{
    for (int i = 0; i < tar_headers->count; ++i)
    {
        if(!strcmp(tar_headers->headers[i]->name, name))
        {
            printf("--BEGIN-- %s\n", name);
            printf("%s", (char *)tar_headers->address[i]);
            printf("--END-- %s\n", name);
            return;
        }
    }
    printf("\033[31mInvalid Filename!\033[0m\n");
}

int initrd_getid(char* name)
{
    for (int i = 0; i < tar_headers->count; ++i)
    {
        if(!strcmp(tar_headers->headers[i]->name, name))
        {
            return i;
        }
    }
    return 0;
}

void initrd_init(unsigned int address)
{
    serial_info("Initializing initrd");

    initrd_parse(address);

    serial_info("Initialized initrd\n");
}
