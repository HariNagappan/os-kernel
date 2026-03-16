#ifndef ISR_H
#define ISR_H

#include <stdint.h>

/*
 * The CPU (and our ASM stubs) push these registers onto the kernel
 * stack before calling the C handler. The struct layout must match
 * the push order in isr_stub.asm exactly.
 */
typedef struct
{
    /* Saved by our stub (pushed in reverse order) */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;

    /* Pushed by our stub: which interrupt fired, and the error code
     * (we push a dummy 0 for exceptions that have no real error code) */
    uint32_t int_no, err_code;

    /* Pushed automatically by the CPU on interrupt */
    uint32_t eip, cs, eflags, user_esp, ss;
} registers_t;

/* C handlers - called from the ASM stubs */
void isr_divide_by_zero(registers_t *regs);     /* INT 0x00 */
void isr_general_protection(registers_t *regs); /* INT 0x0D */
void isr_page_fault(registers_t *regs);         /* INT 0x0E */
void isr_double_fault(registers_t *regs);       /* INT 0x08 */

/* IRQ handler registered by the PIT */
void irq0_timer_handler(registers_t *regs); /* IRQ0 -> INT 0x20 */

#endif