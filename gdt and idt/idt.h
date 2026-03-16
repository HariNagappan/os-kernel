#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/*
 * ============================================================
 *  WHAT IS THE IDT?
 * ============================================================
 *  When ANY interrupt occurs — whether from hardware (keyboard,
 *  timer) or software (divide by zero, page fault, INT n) —
 *  the CPU:
 *    1. Reads the interrupt vector number (0–255)
 *    2. Indexes into the IDT: IDT[vector]
 *    3. Reads the 32-bit handler address from that entry
 *    4. Jumps to that address
 *
 *  We must populate all 256 entries.
 *  Vectors 0–31   : CPU exceptions (defined by Intel)
 *  Vectors 32–47  : Hardware IRQs  (remapped from PIC)
 *  Vectors 48–255 : Free for system calls, software interrupts, etc.
 * ============================================================
 */

/*
 * IDT_ENTRY — one 8-byte interrupt gate descriptor.
 *
 * Memory layout:
 *  ┌──────────────┬──────────┬───────┬───────────┬──────────────┐
 *  │ offset[0:15] │ selector │  zero │ type_attr │ offset[16:31]│
 *  │   2 bytes    │  2 bytes │ 1 byte│  1 byte   │   2 bytes    │
 *  └──────────────┴──────────┴───────┴───────────┴──────────────┘
 *
 * The handler's 32-bit address (offset) is split across
 * offset_low and offset_high — same legacy reason as GDT.
 */
typedef struct {
    uint16_t offset_low;  /* bits  0–15 of the ISR handler address  */
    uint16_t selector;    /* GDT segment selector (must be ring-0 code = 0x08) */
    uint8_t  zero;        /* always 0 — reserved by CPU                        */
    uint8_t  type_attr;   /* gate type + DPL + present bit                     */
    uint16_t offset_high; /* bits 16–31 of the ISR handler address  */
} __attribute__((packed)) idt_entry_t;

/*
 * TYPE_ATTR BYTE  (idt_entry_t.type_attr)
 * ────────────────────────────────────────
 *  Bit 7   (P)  : Present — 1 = valid entry
 *  Bits 6–5(DPL): min privilege to trigger via software INT n
 *                   0 = only kernel can call, 3 = user can call
 *  Bit 4        : Always 0 for interrupt/trap gates
 *  Bits 3–0(type): Gate type
 *                   0xE = 32-bit Interrupt Gate
 *                         → CPU clears IF (disables interrupts) on entry
 *                         → use for hardware IRQs and most exceptions
 *                   0xF = 32-bit Trap Gate
 *                         → CPU does NOT clear IF
 *                         → use for debugging traps (#DB, #BP)
 *
 *  ┌───┬───┬───┬───┬───────────────┐
 *  │ P │DPL│DPL│ 0 │  gate type    │
 *  └───┴───┴───┴───┴───────────────┘
 *    7   6   5   4   3   2   1   0
 */
#define IDT_FLAG_PRESENT        (1 << 7)

#define IDT_FLAG_RING0          (0 << 5)   /* kernel-only interrupt */
#define IDT_FLAG_RING3          (3 << 5)   /* user can trigger via INT n */

#define IDT_GATE_INTERRUPT_32   0x0E       /* clears IF — use for IRQs/exceptions */
#define IDT_GATE_TRAP_32        0x0F       /* keeps IF — use for #DB, #BP         */

/* Pre-built type_attr values */
#define IDT_KERNEL_INTERRUPT  (IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_GATE_INTERRUPT_32)  /* 0x8E */
#define IDT_KERNEL_TRAP       (IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_GATE_TRAP_32)       /* 0x8F */
#define IDT_USER_INTERRUPT    (IDT_FLAG_PRESENT | IDT_FLAG_RING3 | IDT_GATE_INTERRUPT_32)  /* 0xEE */

/*
 * IDT_PTR — loaded into the IDTR hardware register via lidt.
 * Same structure layout as GDT_PTR.
 */
typedef struct {
    uint16_t limit;   /* size of IDT in bytes, minus 1 (256×8 − 1 = 2047) */
    uint32_t base;    /* linear address of idt[] array                      */
} __attribute__((packed)) idt_ptr_t;

/*
 * INTERRUPT_FRAME_T
 * ─────────────────
 * When an interrupt fires, the CPU and our ISR stub together
 * build this structure on the KERNEL stack before calling our C handler.
 *
 * Stack layout at the point our C handler receives the pointer:
 *
 *   [HIGH]
 *   ss          ← only pushed if privilege level changes (e.g. ring3 → ring0)
 *   useresp     ← only pushed on privilege change
 *   eflags      ← always pushed by CPU
 *   cs          ← always pushed by CPU
 *   eip         ← always pushed by CPU (return address)
 *   err_code    ← pushed by CPU (or dummy 0 by our stub)
 *   int_no      ← pushed by our stub
 *   eax,ecx,edx,ebx,esp,ebp,esi,edi ← pushed by pusha in stub
 *   ds          ← pushed by our stub (saved segment register)
 *   [LOW]  ← ESP points here → this is our interrupt_frame_t *
 */
typedef struct {
    uint32_t ds;                                        /* saved DS register            */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;   /* saved by pusha (note order)  */
    uint32_t int_no;                                    /* interrupt vector number       */
    uint32_t err_code;                                  /* error code (0 if none)        */
    /* CPU auto-pushes these on interrupt entry: */
    uint32_t eip;                                       /* instruction that was running  */
    uint32_t cs;                                        /* code segment at time of fault */
    uint32_t eflags;                                    /* CPU flags register            */
    uint32_t useresp;                                   /* user stack pointer (ring3→0)  */
    uint32_t ss;                                        /* user stack segment (ring3→0)  */
} interrupt_frame_t;

/* 256 vectors: 0–31 exceptions, 32–47 IRQs, 48–255 available */
#define IDT_ENTRY_COUNT  256

/* Type of a C-level interrupt handler function */
typedef void (*isr_handler_t)(interrupt_frame_t *frame);

/* Public API */
void idt_init(void);
void idt_set_entry(uint8_t vector, uint32_t handler,
                   uint16_t selector, uint8_t type_attr);
void idt_register_handler(uint8_t vector, isr_handler_t fn);

/* Defined in idt_flush.asm */
extern void idt_flush(uint32_t idt_ptr_address);

/*
 * ISR stubs — defined in isr_stubs.asm.
 * These are the ACTUAL addresses placed into the IDT entries.
 * Each stub pushes int_no + err_code then jumps to isr_common_stub.
 */

/* CPU Exceptions (vectors 0–21) */
extern void isr0(void);    /* #DE  Divide-by-Zero                */
extern void isr1(void);    /* #DB  Debug                         */
extern void isr2(void);    /*      Non-Maskable Interrupt        */
extern void isr3(void);    /* #BP  Breakpoint                    */
extern void isr4(void);    /* #OF  Overflow                      */
extern void isr5(void);    /* #BR  Bound Range Exceeded          */
extern void isr6(void);    /* #UD  Invalid Opcode                */
extern void isr7(void);    /* #NM  Device Not Available          */
extern void isr8(void);    /* #DF  Double Fault      (err code)  */
extern void isr9(void);    /*      Coprocessor Overrun           */
extern void isr10(void);   /* #TS  Invalid TSS       (err code)  */
extern void isr11(void);   /* #NP  Segment Not Present(err code) */
extern void isr12(void);   /* #SS  Stack-Segment Fault(err code) */
extern void isr13(void);   /* #GP  General Protection(err code)  */
extern void isr14(void);   /* #PF  Page Fault        (err code)  */
extern void isr15(void);   /*      Reserved                      */
extern void isr16(void);   /* #MF  x87 FP Exception              */
extern void isr17(void);   /* #AC  Alignment Check   (err code)  */
extern void isr18(void);   /* #MC  Machine Check                 */
extern void isr19(void);   /* #XM  SIMD FP Exception             */
extern void isr20(void);   /* #VE  Virtualization Exception      */
extern void isr21(void);   /*      Reserved                      */

/* Hardware IRQs (vectors 32–47, after PIC remapping) */
extern void irq0(void);    /* IRQ0  — PIT Timer       → vector 32 */
extern void irq1(void);    /* IRQ1  — Keyboard        → vector 33 */
extern void irq2(void);    /* IRQ2  — PIC cascade     → vector 34 */
extern void irq3(void);    /* IRQ3  — COM2            → vector 35 */
extern void irq4(void);    /* IRQ4  — COM1            → vector 36 */
extern void irq5(void);    /* IRQ5  — LPT2            → vector 37 */
extern void irq6(void);    /* IRQ6  — Floppy          → vector 38 */
extern void irq7(void);    /* IRQ7  — LPT1/Spurious   → vector 39 */
extern void irq8(void);    /* IRQ8  — RTC             → vector 40 */
extern void irq9(void);    /* IRQ9  — Free/ACPI       → vector 41 */
extern void irq10(void);   /* IRQ10 — Free            → vector 42 */
extern void irq11(void);   /* IRQ11 — Free            → vector 43 */
extern void irq12(void);   /* IRQ12 — PS/2 Mouse      → vector 44 */
extern void irq13(void);   /* IRQ13 — FPU             → vector 45 */
extern void irq14(void);   /* IRQ14 — Primary ATA     → vector 46 */
extern void irq15(void);   /* IRQ15 — Secondary ATA   → vector 47 */

#endif /* IDT_H */