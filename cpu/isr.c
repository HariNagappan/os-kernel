#include "isr.h"
#include "../log/log.h"
#include "../time/time.h"
#include "../pic/pic.h"

/*
 * isr_unhandled_interrupt
 * ───────────────────────
 * A generic handler for interrupts that don't have a specific C routine
 * registered. This is where you might log unexpected interrupts.
 */
void isr_unhandled_interrupt(registers_t *regs)
{
    log_warn("Unhandled Interrupt (Vector: 0x%x, Error Code: 0x%x, EIP: 0x%x)",
             regs->int_no, regs->err_code, regs->eip);
    // For a real system, you might gracefully shut down the offending task
    // or log more context. For now, we'll just acknowledge and continue.
    // If it's a hardware IRQ, we must send EOI.
    if (regs->int_no >= 32 && regs->int_no <= 47) {
        pic_send_eoi(regs->int_no - 32);
    }
}

/*
 * INT 0x00 - Divide By Zero Exception
 * ────────────────────────────────────
 * Fires when code executes DIV or IDIV with a zero divisor.
 * This is a critical error, usually unrecoverable.
 */
void isr_divide_by_zero(registers_t *regs)
{
    log_fatal("EXCEPTION #0: Divide By Zero at EIP=0x%x (CS:0x%x)", regs->eip, regs->cs);
    log_fatal("  Attempted division by zero detected. System halting.");
    __asm__ volatile("cli; hlt"); // Disable interrupts and halt CPU
}

/*
 * INT 0x06 - Invalid Opcode Exception
 * ───────────────────────────────────
 * Fires when the CPU encounters an instruction that is not valid.
 * This indicates corrupted code or a serious programming error.
 */
void isr_invalid_opcode(registers_t *regs)
{
    log_fatal("EXCEPTION #6: Invalid Opcode at EIP=0x%x (CS:0x%x)", regs->eip, regs->cs);
    log_fatal("  The CPU attempted to execute an invalid instruction. System halting.");
    __asm__ volatile("cli; hlt"); // Disable interrupts and halt CPU
}

/*
 * INT 0x08 - Double Fault Exception
 * ─────────────────────────────────
 * Fires when the CPU encounters an exception while trying to
 * handle another exception. This typically indicates a severely
 * corrupted stack or an issue in an exception handler itself.
 */
void isr_double_fault(registers_t *regs)
{
    log_fatal("EXCEPTION #8: Double Fault (Error Code: 0x%x) at EIP=0x%x (CS:0x%x)",
              regs->err_code, regs->eip, regs->cs);
    log_fatal("  A critical exception occurred while handling another exception. This is unrecoverable.");
    __asm__ volatile("cli; hlt"); // Disable interrupts and halt CPU
}

/*
 * INT 0x0D - General Protection Fault Exception
 * ─────────────────────────────────────────────
 * Fires on segment violations, privilege violations, invalid memory accesses
 * (e.g., writing to a read-only segment), invalid TSS operations, etc.
 * The error code provides information about the cause of the fault.
 */
void isr_general_protection(registers_t *regs)
{
    log_fatal("EXCEPTION #13: General Protection Fault (Error Code: 0x%x) at EIP=0x%x (CS:0x%x)",
              regs->err_code, regs->eip, regs->cs);
    log_fatal("  Possible causes: segment violation, privilege error, or invalid memory access.");

    // Further analysis of err_code can be done here to pinpoint the cause
    // e.g., external/internal, GDT/LDT, index.
    if (regs->err_code & 0x1) log_fatal("    - External event");
    if (regs->err_code & 0x2) log_fatal("    - IDT");
    if (regs->err_code & 0x4) log_fatal("    - GDT");

    __asm__ volatile("cli; hlt"); // Disable interrupts and halt CPU
}

/*
 * INT 0x0E - Page Fault Exception
 * ───────────────────────────────
 * Fires when the CPU tries to access a virtual address that has no valid
 * mapping in the page tables, or the access violates defined permissions.
 * CR2 register holds the linear (virtual) address that caused the fault.
 * The error code (pushed by CPU) describes the nature of the fault.
 */
void isr_page_fault(registers_t *regs)
{
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));

    int present = regs->err_code & 0x01;  // P = 0 means page not present, P = 1 means protection violation
    int write = regs->err_code & 0x02;    // W = 0 means read access, W = 1 means write access
    int user = regs->err_code & 0x04;     // U = 0 means kernel mode, U = 1 means user mode
    int reserved = regs->err_code & 0x08; // R = 1 means reserved bits overwritten
    int instruction_fetch = regs->err_code & 0x10; // I = 1 means fault was during instruction fetch

    log_fatal("EXCEPTION #14: Page Fault at LINEAR_ADDR=0x%x, EIP=0x%x (CS:0x%x)",
              fault_addr, regs->eip, regs->cs);
    log_fatal("  Error Code: 0x%x (Flags: %s %s %s %s %s)",
              regs->err_code,
              present ? "PROTECTION-VIOLATION" : "PAGE-NOT-PRESENT",
              write ? "WRITE-ACCESS" : "READ-ACCESS",
              user ? "USER-MODE" : "KERNEL-MODE",
              reserved ? "RESERVED-BITS-OVERWRITTEN" : "",
              instruction_fetch ? "INSTRUCTION-FETCH" : "");

    // In a full kernel, this would involve memory allocation (demand paging)
    // or killing the process. For now, it's a fatal error.
    __asm__ volatile("cli; hlt"); // Disable interrupts and halt CPU
}

/*
 * IRQ0 -> INT 0x20 - PIT Timer Handler
 * ────────────────────────────────────
 * This function is called by the `isr_common_handler` every time
 * the Programmable Interval Timer (PIT) fires IRQ0.
 *
 * It increments a global tick counter, which is essential for timekeeping
 * and later for scheduling tasks in the OS.
 *
 * It MUST send an End-Of-Interrupt (EOI) signal to the PICs to acknowledge
 * the interrupt, otherwise the PIC will not send further interrupts.
 */
void irq0_timer_handler(registers_t *regs)
{
    (void)regs; // Parameter 'regs' is not used in this basic handler, so cast to void to suppress warnings.

    time_set_ticks(time_get_ticks() + 1); // Increment the global system tick counter

    // Send EOI to the Master PIC for IRQ0
    pic_send_eoi(0);

    // In a more advanced kernel, a scheduler might be invoked here
    // to switch to another task, or other time-based events could be processed.
}