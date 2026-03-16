#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* dest, int value, size_t n);
size_t strlen(const char *str);
int strcmp(const char *a, const char *b);
char *strcpy(char *dest, const char *src);
int memcmp(const void *a, const void *b, size_t n);

#endif