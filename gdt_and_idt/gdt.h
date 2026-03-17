#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/*
 * ============================================================
 *  WHAT IS THE GDT?
 * ============================================================
 *  The CPU does not work with raw pointers in protected mode.
 *  Every memory access goes through a "segment selector" which
 *  is an index into this table. Each entry (descriptor) tells
 *  the CPU:
 *    - WHERE the segment starts (base address)
 *    - HOW LARGE it is (limit)
 *    - WHO can use it (ring 0 = kernel, ring 3 = user)
 *    - WHAT it is (code or data)
 *
 *  We use a FLAT MEMORY MODEL — both code and data segments
 *  cover the full 4 GB address space (base=0, limit=0xFFFFF
 *  with 4 KB granularity). Privilege separation is done purely
 *  through the DPL (Descriptor Privilege Level) bits.
 *
 *  Our 5 descriptors:
 *    [0] NULL         — required by CPU spec (must be zero)
 *    [1] Kernel Code  — ring 0, executable, readable
 *    [2] Kernel Data  — ring 0, writable
 *    [3] User   Code  — ring 3, executable, readable
 *    [4] User   Data  — ring 3, writable
 * ============================================================
 */

/*
 * GDT_ENTRY — one 8-byte segment descriptor.
 *
 * The hardware layout is NOT linear. The base address and limit
 * are intentionally split across multiple fields (legacy 286 design).
 *
 *  Byte layout of one entry (8 bytes total):
 *  ┌──────────┬──────────┬─────────┬────────┬─────────────┬──────────┐
 *  │limit[0:15]│base[0:15]│base[16:23]│access │gran+lim[16:19]│base[24:31]│
 *  │  2 bytes  │  2 bytes │  1 byte │ 1 byte │    1 byte   │  1 byte  │
 *  └──────────┴──────────┴─────────┴────────┴─────────────┴──────────┘
 */
typedef struct {
    uint16_t limit_low;   /* bits  0–15  of the 20-bit segment limit        */
    uint16_t base_low;    /* bits  0–15  of the 32-bit base address          */
    uint8_t  base_mid;    /* bits 16–23  of the base address                 */
    uint8_t  access;      /* access byte — controls permissions & type       */
    uint8_t  granularity; /* high nibble = flags, low nibble = limit bits 16–19 */
    uint8_t  base_high;   /* bits 24–31  of the base address                 */
} __attribute__((packed)) gdt_entry_t;
/*
 * __attribute__((packed)):
 *   Without this, GCC adds padding bytes for alignment.
 *   The CPU reads raw bytes at exact offsets — padding breaks it.
 */

/*
 * ACCESS BYTE  (gdt_entry_t.access)
 * ─────────────────────────────────
 *  Bit 7   (P)  : Present — must be 1 for any valid descriptor
 *  Bit 6–5 (DPL): Descriptor Privilege Level
 *                   00 = ring 0 (kernel)
 *                   11 = ring 3 (user)
 *  Bit 4   (S)  : Descriptor type — 1 = code/data, 0 = system
 *  Bit 3   (E)  : Executable — 1 = code segment, 0 = data segment
 *  Bit 2   (DC) : Direction/Conforming (0 = normal)
 *  Bit 1   (RW) : Readable (code) / Writable (data)
 *  Bit 0   (A)  : Accessed — CPU sets this; we leave it 0
 *
 *  Visual:
 *  ┌───┬───┬───┬───┬───┬───┬───┬───┐
 *  │ P │DPL│DPL│ S │ E │DC │RW │ A │
 *  └───┴───┴───┴───┴───┴───┴───┴───┘
 *    7   6   5   4   3   2   1   0
 */
#define GDT_ACCESS_PRESENT      (1 << 7)  /* 0x80 — segment is present in memory  */
#define GDT_ACCESS_RING0        (0 << 5)  /* 0x00 — kernel privilege level        */
#define GDT_ACCESS_RING3        (3 << 5)  /* 0x60 — user privilege level          */
#define GDT_ACCESS_DESCRIPTOR   (1 << 4)  /* 0x10 — code or data segment          */
#define GDT_ACCESS_EXECUTABLE   (1 << 3)  /* 0x08 — code (executable) segment     */
#define GDT_ACCESS_RW           (1 << 1)  /* 0x02 — readable (code) / writable (data) */

/* Pre-built access bytes for each of our 4 real segments */
#define GDT_KERNEL_CODE_ACCESS \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR | \
     GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW)   /* = 0x9A */

#define GDT_KERNEL_DATA_ACCESS \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR | \
     GDT_ACCESS_RW)                            /* = 0x92 */

#define GDT_USER_CODE_ACCESS \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR | \
     GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW)   /* = 0xFA */

#define GDT_USER_DATA_ACCESS \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR | \
     GDT_ACCESS_RW)                            /* = 0xF2 */

/*
 * GRANULARITY BYTE  (gdt_entry_t.granularity)
 * ─────────────────────────────────────────────
 *  High nibble (bits 7–4) — flags:
 *    Bit 7 (G) : Granularity — 0 = limit in bytes, 1 = limit in 4 KB pages
 *    Bit 6 (D) : Default size — 1 = 32-bit protected mode
 *    Bit 5 (L) : Long mode — 1 = 64-bit (then D must be 0)
 *    Bit 4     : Reserved, always 0
 *  Low nibble (bits 3–0) — limit bits 16–19
 *
 *  ┌───┬───┬───┬───┬───┬───┬───┬───┐
 *  │ G │ D │ L │ 0 │     limit     │
 *  └───┴───┴───┴───┴───────────────┘
 *    7   6   5   4   3   2   1   0
 */
#define GDT_GRAN_4K      (1 << 7)  /* scale limit by 4096 → covers 4 GB */
#define GDT_GRAN_32BIT   (1 << 6)  /* 32-bit protected mode              */
/* Combined flag for all our segments */
#define GDT_FLAGS        (GDT_GRAN_4K | GDT_GRAN_32BIT)  /* = 0xCF when OR'd with limit high nibble */

/*
 * GDT_PTR — loaded into the GDTR hardware register via lgdt.
 * Tells the CPU where the GDT lives and how large it is.
 *
 *  ┌─────────────┬──────────────────────┐
 *  │  limit (2B) │     base (4B)        │
 *  └─────────────┴──────────────────────┘
 */
typedef struct {
    uint16_t limit;  /* size of GDT in bytes, minus 1                  */
    uint32_t base;   /* linear (physical) address of the gdt[] array   */
} __attribute__((packed)) gdt_ptr_t;

/*
 * Segment selector values used to reload segment registers.
 * Format: bits[15:3] = GDT index, bit[2] = TI (0=GDT), bits[1:0] = RPL
 *
 *   Selector = (index << 3) | RPL
 *
 *   0x08 = index 1, RPL 0 → Kernel Code
 *   0x10 = index 2, RPL 0 → Kernel Data
 *   0x18 = index 3, RPL 0 → User   Code  (note: RPL would be 3 in real use)
 *   0x20 = index 4, RPL 0 → User   Data
 */
#define SEG_KERNEL_CODE  0x08
#define SEG_KERNEL_DATA  0x10
#define SEG_USER_CODE    0x18
#define SEG_USER_DATA    0x20

#define GDT_ENTRY_COUNT  5   /* null + kernel code/data + user code/data */

/* Public API */
void gdt_init(void);
void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t granularity);

/* Defined in gdt_flush.asm — reloads GDTR and all segment registers */
extern void gdt_flush(uint32_t gdt_ptr_address);

#endif /* GDT_H */