#include <stdint.h>
#include <stddef.h>

uint32_t strlen(const char* str)
{
    uint32_t length = 0;
    while(str[length])
    {
        length++;
    }
    return length;
}

char* strcpy(char* destination, const char* source)
{
    if (destination == NULL)
    {
        return NULL;
    }
    char *ptr = destination;
    while (*source != '\0')
    {
        *destination = *source;
        destination++;
        source++;
    }
    *destination = '\0';
    return ptr;
}

char* strcat(char* destination, const char* source)
{
    char* ptr = destination + strlen(destination);
    while (*source != '\0')
    {
        *ptr++ = *source++;
    }
    *ptr = '\0';
    return destination;
}

void memcpy(void *dest, void *src, size_t n) 
{
    int i;
    char *src_char = (char *)src;
    char *dest_char = (char *)dest;
    for (i=0; i<n; i++)
    {
        dest_char[i] = src_char[i];
    }
}

void memset(void* str, char ch, size_t n)
{
  	int i;
	  char *s = (char*) str;
	  for(i=0; i<n; i++)
    {
        s[i]=ch;
    }
}