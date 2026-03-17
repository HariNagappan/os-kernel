#ifndef PMM_H
#define PMM_H

#include "../types/types.h"
#include "../memory_map/memory_map.h"

/* Simulated physical memory: 64 MB */
#define PHYS_MEM_SIZE     MB(64)

/* Bitmap covers PHYS_MEM_SIZE / PAGE_SIZE bits
 * 64MB / 4KB = 16384 pages → 16384 / 8 = 2048 bytes bitmap */
#define PMM_MAX_PAGES     (PHYS_MEM_SIZE / PAGE_SIZE)
#define PMM_BITMAP_BYTES  (PMM_MAX_PAGES / 8)

typedef struct {
    u8   bitmap[PMM_BITMAP_BYTES];  /* 1 = used, 0 = free */
    u64  total_pages;
    u64  free_pages;
    u64  used_pages;
} pmm_t;

/* Init / destroy */
void    pmm_init       (pmm_t *pmm, const memory_map_t *mmap);

/* Allocation */
paddr_t pmm_alloc      (pmm_t *pmm);                  /* alloc 1 page  */
paddr_t pmm_alloc_n    (pmm_t *pmm, u64 n);           /* alloc n contiguous pages */
void    pmm_free       (pmm_t *pmm, paddr_t addr);    /* free 1 page   */
void    pmm_free_n     (pmm_t *pmm, paddr_t addr, u64 n);

/* Helpers */
void    pmm_print_stats(const pmm_t *pmm);
int     pmm_is_free    (const pmm_t *pmm, paddr_t addr);

/* Physical <-> pointer conversion (simulated memory) */
void   *phys_to_virt   (paddr_t addr);
paddr_t virt_to_phys   (void *ptr);

#endif /* PMM_H */
