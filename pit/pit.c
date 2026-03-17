#include "pit.h"
#include "../pic/pic.h"

/*
 * The PIT has a fixed input clock of 1,193,182 Hz.
 * We tell it a divisor; it fires IRQ0 every (divisor) clock ticks.
 *
 *   divisor = 1,193,182 / desired_hz
 *
 * Example: 1,193,182 / 100 = 11931  -> IRQ0 fires 100 times per second.
 */
#define PIT_BASE_FREQ 1193182

/* PIT I/O ports */
#define PIT_DATA_CH0 0x40 /* Channel 0 data port (read/write) */
#define PIT_CMD 0x43      /* Mode/command register (write only) */

/*
 * Command byte breakdown (we send 0x36):
 *   Bits 7-6: 00  = channel 0
 *   Bits 5-4: 11  = lo byte then hi byte
 *   Bits 3-1: 011 = mode 3 (square wave generator)
 *   Bit  0:   0   = binary counting
 */
#define PIT_CMD_BINARY_MODE3 0x36

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void pit_init(uint32_t hz)
{
    uint32_t divisor = PIT_BASE_FREQ / hz;

    /* Set operating mode */
    outb(PIT_CMD, PIT_CMD_BINARY_MODE3);

    /* Send divisor as two bytes: low byte first, then high byte */
    outb(PIT_DATA_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_DATA_CH0, (uint8_t)((divisor >> 8) & 0xFF));

    /* Unmask IRQ0 so the PIC lets timer interrupts through */
    pic_unmask_irq(0);
}