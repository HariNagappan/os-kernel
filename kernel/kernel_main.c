#include "drivers/vga/vga.h"
#include "lib/printf.h"
#include "log/log.h"
#include "pic/pic.h"
#include "pit/pit.h"
#include "time/time.h"

/*
 * These are the raw ASM entry points defined in cpu/isr_stub.asm.
 * The IDT (built in Phase 2) stores their addresses so the CPU knows
 * where to jump when each interrupt fires.
 * We declare them here as extern so the linker can resolve them.
 */
extern void isr0_stub();  /* INT 0x00 - Divide By Zero        */
extern void isr8_stub();  /* INT 0x08 - Double Fault          */
extern void isr13_stub(); /* INT 0x0D - General Protection    */
extern void isr14_stub(); /* INT 0x0E - Page Fault            */
extern void irq0_stub();  /* INT 0x20 - PIT Timer (IRQ0)      */

/* ---------------------------------------------------------------
 * IDT - built in Phase 2, declared here so we can install stubs.
 * Replace this block with your actual idt_set_gate() / idt_load()
 * calls once your IDT code is written.
 * --------------------------------------------------------------- */
// idt_set_gate(0,  (uint32_t)isr0_stub,  0x08, 0x8E);
// idt_set_gate(8,  (uint32_t)isr8_stub,  0x08, 0x8E);
// idt_set_gate(13, (uint32_t)isr13_stub, 0x08, 0x8E);
// idt_set_gate(14, (uint32_t)isr14_stub, 0x08, 0x8E);
// idt_set_gate(32, (uint32_t)irq0_stub,  0x08, 0x8E); /* 0x20 = 32 */
// idt_load();

/* ---------------------------------------------------------------
 * A simple busy-wait using the tick counter as a time source.
 * Waits until `ms` milliseconds worth of ticks have passed.
 * At 100 Hz, 1 tick = 10 ms, so ms is rounded to the nearest 10.
 * --------------------------------------------------------------- */
static void sleep_ms(uint32_t ms)
{
    uint64_t start = time_get_ticks();
    uint64_t target = start + (ms / 10); /* 100 Hz -> 1 tick per 10 ms */
    while (time_get_ticks() < target)
        __asm__ volatile("hlt"); /* sleep until next interrupt */
}

/* ---------------------------------------------------------------
 * Kernel entry point.
 * Your bootloader (e.g. GRUB via Multiboot) jumps here after
 * setting up a basic stack and loading the kernel image.
 * --------------------------------------------------------------- */
void kernel_main()
{
    /* 1. Clear the screen and say hello */
    vga_init();
    log_info("Kernel booting...");

    /* 2. Remap the PIC so hardware IRQs don't collide with CPU exceptions */
    pic_init();
    log_info("PIC remapped: IRQ0-7 -> INT 0x20-0x27, IRQ8-15 -> INT 0x28-0x2F");

    /* 3. Install ISR stubs into the IDT (Phase 2 code goes here)
     *    Uncomment the idt_set_gate / idt_load calls above once your
     *    IDT implementation is ready. */
    log_info("IDT stubs ready (install via idt_set_gate in Phase 2)");

    /* 4. Configure the PIT to fire IRQ0 at 100 Hz
     *    Each tick calls irq0_timer_handler -> time_set_ticks(ticks + 1) */
    pit_init(100);
    log_info("PIT configured: 100 Hz (1 tick = 10 ms)");

    /* 5. Enable interrupts - from this point the timer starts ticking */
    __asm__ volatile("sti");
    log_info("Interrupts enabled. Tick counter is live.");

    /* 6. Show the tick counter incrementing so you can confirm it works */
    log_info("Watching ticks for 5 seconds...");
    for (int i = 0; i < 5; i++)
    {
        sleep_ms(1000);
        log_info("  tick = %d", (int)time_get_ticks());
    }

    /* 7. Kernel idle loop - hlt saves power between interrupts */
    log_info("Boot complete. Entering idle loop.");
    for (;;)
        __asm__ volatile("hlt");
}