#ifndef KMALLOC_H
#define KMALLOC_H

#include "types.h"

typedef struct block_header {
    uint32_t size;
    uint8_t  is_free;
    struct block_header* next;
} block_header_t;

void  kmalloc_init(uint32_t start_addr, uint32_t size);
void* kmalloc(uint32_t size);
void  kfree(void* ptr);

#endif