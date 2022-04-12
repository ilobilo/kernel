#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int abs(int num);
int sign(int num);

uint64_t rand();
void srand(uint64_t seed);

long strtol(const char *nPtr, char **endPtr, int base);
long long int strtoll(const char *nPtr, char **endPtr, int base);
unsigned long strtoul(const char *nPtr, char **endPtr, int base);
unsigned long long int strtoull(const char *nPtr, char **endPtr, int base);

void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

#ifdef __cplusplus
}
#endif