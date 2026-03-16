#include "gdt.h"

/*
 * gdt[] — the actual table that lives in memory.
 * 'static' keeps it in this translation unit only.
 * The CPU reads these bytes directly when it processes
 * any memory reference or far jump.
 */
static gdt_entry_t gdt[GDT_ENTRY_COUNT];

/*
 * gdt_ptr — the 6-byte structure we load into GDTR.
 * After gdt_init() calls gdt_flush(), the CPU knows
 * about our GDT at this address.
 */
static gdt_ptr_t gdt_ptr;

/*
 * gdt_set_entry()
 * ───────────────
 * Encodes a segment descriptor into gdt[index].
 *
 * Parameters:
 *   index       — which slot (0–4) to fill
 *   base        — 32-bit linear start address of segment
 *   limit       — 20-bit size (in pages if G=1, bytes if G=0)
 *   access      — access byte (permissions, type, ring)
 *   granularity — flags byte (G, D/B, L bits + upper limit nibble)
 *
 * Why the splitting?
 *   Intel's 286 designers packed these fields this way.
 *   We must reassemble them into the exact byte positions
 *   the hardware expects.
 */
void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t granularity)
{
    gdt_entry_t *e = &gdt[index];

    /*
     * BASE ADDRESS — split across 3 non-contiguous fields:
     *
     *  base_low  ← bits  0–15  (mask with 0x0000FFFF)
     *  base_mid  ← bits 16–23  (shift right 16, mask with 0xFF)
     *  base_high ← bits 24–31  (shift right 24, mask with 0xFF)
     */
    e->base_low  =  base        & 0xFFFF;
    e->base_mid  = (base >> 16) & 0xFF;
    e->base_high = (base >> 24) & 0xFF;

    /*
     * SEGMENT LIMIT — 20 bits total, split across 2 fields:
     *
     *  limit_low  ← bits  0–15  (mask with 0xFFFF)
     *  granularity (low nibble) ← bits 16–19 (shift right 16, mask with 0x0F)
     *
     *  The caller also passes flag bits in the HIGH nibble of
     *  'granularity' (G, D, L, reserved). We OR them together
     *  so one byte holds both flags and the limit's top 4 bits.
     */
    e->limit_low    =  limit        & 0xFFFF;
    e->granularity  = (limit >> 16) & 0x0F;   /* limit bits 16–19 into low nibble  */
    e->granularity |=  granularity  & 0xF0;   /* G/D/L flags into high nibble      */

    e->access = access;
}

/*
 * gdt_init()
 * ──────────
 * Build all 5 descriptors, then load the GDT into the CPU.
 *
 * Flat Memory Model:
 *   Every segment: base = 0x00000000, limit = 0xFFFFF (with G=1 → 4 GB).
 *   No segment isolation — protection comes from ring levels only.
 *   This is standard for modern 32-bit OS kernels.
 *
 * Segment Selector Map (index × 8 = selector value):
 *   0x00 — NULL
 *   0x08 — Kernel Code   (ring 0, executable)
 *   0x10 — Kernel Data   (ring 0, writable)
 *   0x18 — User   Code   (ring 3, executable)
 *   0x20 — User   Data   (ring 3, writable)
 */
void gdt_init(void)
{
    /*
     * Point GDTR at our gdt[] array.
     * limit = (number of entries × 8 bytes) − 1
     */
    gdt_ptr.limit = (uint16_t)(sizeof(gdt_entry_t) * GDT_ENTRY_COUNT - 1);
    gdt_ptr.base  = (uint32_t)&gdt;

    /* ── [0] NULL Descriptor ─────────────────────────────────────────
     *  Required by the CPU specification.
     *  Loading any segment register with selector 0x00 causes a #GP.
     *  All fields are zero.
     */
    gdt_set_entry(0,
        0x00000000,   /* base  */
        0x00000000,   /* limit */
        0x00,         /* access      — all zero */
        0x00          /* granularity — all zero */
    );

    /* ── [1] Kernel Code Segment ─────────────────────────────────────
     *  Ring 0, executable, readable.
     *  Covers the full 4 GB (flat model).
     *  Loaded into CS by the far jump in gdt_flush.asm.
     */
    gdt_set_entry(1,
        0x00000000,              /* base  — starts at address 0    */
        0x000FFFFF,              /* limit — 0xFFFFF pages × 4 KB = 4 GB */
        GDT_KERNEL_CODE_ACCESS,  /* 0x9A — present, ring 0, code, readable */
        GDT_FLAGS                /* 0xC0 — 4K granularity, 32-bit mode     */
    );

    /* ── [2] Kernel Data Segment ─────────────────────────────────────
     *  Ring 0, writable, not executable.
     *  Used for DS, ES, FS, GS, SS in kernel mode.
     */
    gdt_set_entry(2,
        0x00000000,
        0x000FFFFF,
        GDT_KERNEL_DATA_ACCESS,  /* 0x92 — present, ring 0, data, writable */
        GDT_FLAGS
    );

    /* ── [3] User Code Segment ───────────────────────────────────────
     *  Ring 3, executable, readable.
     *  User-space processes run with CS = 0x1B (0x18 | RPL 3).
     */
    gdt_set_entry(3,
        0x00000000,
        0x000FFFFF,
        GDT_USER_CODE_ACCESS,    /* 0xFA — present, ring 3, code, readable */
        GDT_FLAGS
    );

    /* ── [4] User Data Segment ───────────────────────────────────────
     *  Ring 3, writable, not executable.
     *  User-space DS/ES/FS/GS/SS = 0x23 (0x20 | RPL 3).
     */
    gdt_set_entry(4,
        0x00000000,
        0x000FFFFF,
        GDT_USER_DATA_ACCESS,    /* 0xF2 — present, ring 3, data, writable */
        GDT_FLAGS
    );

    /*
     * Hand the address of gdt_ptr to assembly.
     * gdt_flush will:
     *   1. lgdt [gdt_ptr]  — load GDTR
     *   2. far jmp         — reload CS with 0x08 (kernel code)
     *   3. mov ax, 0x10    — reload DS/ES/FS/GS/SS with kernel data
     */
    gdt_flush((uint32_t)&gdt_ptr);
}