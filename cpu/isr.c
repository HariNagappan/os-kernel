#include "isr.h"
#include "../log/log.h"
#include "../time/time.h"
#include "../pic/pic.h"

/*
 * INT 0x00 - Divide By Zero
 * Fires when code executes DIV or IDIV with a zero divisor.
 */
void isr_divide_by_zero(registers_t *regs)
{
    log_error("Exception #0: Divide By Zero at EIP=0x%x", regs->eip);

    /* Halt - this exception is unrecoverable */
    __asm__ volatile("cli; hlt");
}

/*
 * INT 0x08 - Double Fault
 * Fires when the CPU encounters an exception while trying to
 * handle another exception. Almost always means the stack is corrupt.
 */
void isr_double_fault(registers_t *regs)
{
    log_error("Exception #8: Double Fault (err=0x%x) at EIP=0x%x",
              regs->err_code, regs->eip);

    __asm__ volatile("cli; hlt");
}

/*
 * INT 0x0D - General Protection Fault
 * Fires on segment violations, privilege violations, invalid opcodes, etc.
 * The error code encodes which segment selector caused the fault.
 */
void isr_general_protection(registers_t *regs)
{
    log_error("Exception #13: General Protection Fault (err=0x%x) at EIP=0x%x",
              regs->err_code, regs->eip);

    __asm__ volatile("cli; hlt");
}

/*
 * INT 0x0E - Page Fault
 * Fires when the CPU accesses a virtual address with no valid mapping,
 * or with wrong permissions. CR2 holds the faulting address.
 * Error code bits: P=present, W=write, U=user, R=reserved, I=instruction.
 */
void isr_page_fault(registers_t *regs)
{
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));

    int present = regs->err_code & 0x01;  /* page was present */
    int write = regs->err_code & 0x02;    /* was a write      */
    int user = regs->err_code & 0x04;     /* came from Ring 3 */
    int reserved = regs->err_code & 0x08; /* reserved bit set */

    log_error("Exception #14: Page Fault at addr=0x%x  EIP=0x%x", fault_addr, regs->eip);
    log_error("  Flags: %s %s %s %s",
              present ? "PROTECTION" : "NOT-PRESENT",
              write ? "WRITE" : "READ",
              user ? "USER" : "KERNEL",
              reserved ? "RESERVED" : "");

    __asm__ volatile("cli; hlt");
}

/*
 * IRQ0 -> INT 0x20 - PIT Timer
 * Called 100 times per second (100 Hz) once the PIT is configured.
 * Increments the global tick counter and sends EOI to the PIC
 * so the PIC will keep firing this interrupt.
 */
void irq0_timer_handler(registers_t *regs)
{
    (void)regs; /* not needed for a basic tick */

    time_set_ticks(time_get_ticks() + 1);

    /* Must send EOI or the PIC silently stops firing IRQ0 */
    pic_send_eoi(0);
}