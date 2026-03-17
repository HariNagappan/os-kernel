#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uintptr_t paddr_t;   // physical address
typedef uintptr_t vaddr_t;   // virtual address

#define PAGE_SIZE        4096ULL
#define PAGE_SHIFT       12
#define PAGE_MASK        (~(PAGE_SIZE - 1))

#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define ALIGN_UP(x, a)   (((x) + (a) - 1) & ~((a) - 1))

#define KB(x) ((x) * 1024ULL)
#define MB(x) ((x) * 1024ULL * 1024ULL)

#endif /* TYPES_H */
