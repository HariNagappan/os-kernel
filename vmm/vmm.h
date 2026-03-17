#ifndef VMM_H
#define VMM_H

#include "../types/types.h"
#include "../pmm/pmm.h"

/*
 * x86-64 4-Level Paging Layout
 *
 * Virtual Address (48-bit canonical):
 * [63:48] sign extension
 * [47:39] PML4 index  (9 bits)
 * [38:30] PDPT  index (9 bits)
 * [29:21] PD    index (9 bits)
 * [20:12] PT    index (9 bits)
 * [11:0 ] Page  offset (12 bits)
 */

#define PML4_SHIFT     39
#define PDPT_SHIFT     30
#define PD_SHIFT       21
#define PT_SHIFT       12
#define ENTRY_MASK     0x1FFULL

#define PML4_INDEX(va)  (((vaddr_t)(va) >> PML4_SHIFT) & ENTRY_MASK)
#define PDPT_INDEX(va)  (((vaddr_t)(va) >> PDPT_SHIFT) & ENTRY_MASK)
#define PD_INDEX(va)    (((vaddr_t)(va) >> PD_SHIFT)   & ENTRY_MASK)
#define PT_INDEX(va)    (((vaddr_t)(va) >> PT_SHIFT)   & ENTRY_MASK)
#define PAGE_OFFSET(va) ((vaddr_t)(va) & (PAGE_SIZE - 1))

/* Page Table Entry flags */
#define PTE_PRESENT     (1ULL << 0)   /* Page is present          */
#define PTE_WRITABLE    (1ULL << 1)   /* Page is writable         */
#define PTE_USER        (1ULL << 2)   /* User accessible          */
#define PTE_WRITE_THRU  (1ULL << 3)   /* Write-through caching    */
#define PTE_NO_CACHE    (1ULL << 4)   /* Disable cache            */
#define PTE_ACCESSED    (1ULL << 5)   /* CPU sets on access       */
#define PTE_DIRTY       (1ULL << 6)   /* CPU sets on write        */
#define PTE_HUGE        (1ULL << 7)   /* 2MB huge page (PD level) */
#define PTE_GLOBAL      (1ULL << 8)   /* Global page (TLB stays)  */
#define PTE_NX          (1ULL << 63)  /* No-execute               */

#define PTE_ADDR_MASK   0x000FFFFFFFFFF000ULL

typedef u64 pte_t;

/* Page map flags shorthand */
#define MAP_KERNEL      (PTE_PRESENT | PTE_WRITABLE)
#define MAP_USER        (PTE_PRESENT | PTE_WRITABLE | PTE_USER)
#define MAP_READONLY    (PTE_PRESENT)
#define MAP_NOEXEC      (PTE_PRESENT | PTE_WRITABLE | PTE_NX)

typedef struct {
    pte_t  *pml4;       /* Root page table (physical addr viewed as ptr) */
    paddr_t pml4_phys;  /* Physical address of PML4                      */
    pmm_t  *pmm;        /* PMM to allocate page table pages from          */
    u64     mapped;     /* Total pages mapped                             */
} vmm_t;

/* Init */
void    vmm_init        (vmm_t *vmm, pmm_t *pmm);

/* Mapping */
int     vmm_map         (vmm_t *vmm, vaddr_t virt, paddr_t phys, u64 flags);
int     vmm_map_range   (vmm_t *vmm, vaddr_t virt, paddr_t phys, u64 pages, u64 flags);
int     vmm_unmap       (vmm_t *vmm, vaddr_t virt);
int     vmm_unmap_range (vmm_t *vmm, vaddr_t virt, u64 pages);

/* Translation */
paddr_t vmm_translate   (vmm_t *vmm, vaddr_t virt);
int     vmm_is_mapped   (vmm_t *vmm, vaddr_t virt);

/* Debug */
void    vmm_print_mapping (vmm_t *vmm, vaddr_t virt);
void    vmm_print_stats   (const vmm_t *vmm);
void    vmm_dump_pml4     (vmm_t *vmm);

#endif /* VMM_H */
