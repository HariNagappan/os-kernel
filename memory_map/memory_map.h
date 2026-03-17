#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include "../types/types.h"

#define MMAP_MAX_ENTRIES  64

/* Memory region types (mirrors BIOS e820 types) */
typedef enum {
    MMAP_FREE       = 1,   /* Usable RAM                */
    MMAP_RESERVED   = 2,   /* Reserved by hardware/BIOS */
    MMAP_ACPI       = 3,   /* ACPI reclaimable          */
    MMAP_NVS        = 4,   /* ACPI NVS                  */
    MMAP_BADRAM     = 5,   /* Defective RAM             */
    MMAP_KERNEL     = 6,   /* Kernel + modules          */
} mmap_type_t;

typedef struct {
    u64         base;
    u64         length;
    mmap_type_t type;
} mmap_entry_t;

typedef struct {
    mmap_entry_t entries[MMAP_MAX_ENTRIES];
    u32          count;
    u64          total_free;    /* total free bytes   */
    u64          total_usable;  /* total usable bytes */
} memory_map_t;

void        mmap_init      (memory_map_t *mmap);
void        mmap_add_entry (memory_map_t *mmap, u64 base, u64 length, mmap_type_t type);
void        mmap_parse     (memory_map_t *mmap);
void        mmap_print     (const memory_map_t *mmap);
const char *mmap_type_str  (mmap_type_t type);

#endif /* MEMORY_MAP_H */
