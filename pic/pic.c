#include "pic.h"

/* Master PIC: command=0x20, data=0x21 */
/* Slave  PIC: command=0xA0, data=0xA1 */
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

/* End-of-interrupt command */
#define PIC_EOI 0x20

/* ICW1: start init sequence, expect ICW4 */
#define ICW1_INIT 0x11

/* ICW4: 8086 mode */
#define ICW4_8086 0x01
#define PIC1 0x20
#define PIC2 0xA0

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Small delay by doing a dummy write to port 0x80 */
static inline void io_wait()
{
    outb(0x80, 0x00);
}

/*
 * pic_init - Remap the two 8259 PIC chips.
 *
 * By default, the master PIC fires IRQ0-7 as INT 0x08-0x0F.
 * Those slots are already used by CPU exceptions (double fault,
 * GPF, etc.), so every hardware interrupt would look like a CPU
 * crash. We remap:
 *   Master IRQ0-7  -> INT 0x20-0x27
 *   Slave  IRQ8-15 -> INT 0x28-0x2F
 */
void pic_init()
{
    /* Save current masks so we can restore them after remapping */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* ICW1: tell both PICs to start the init sequence */
    outb(PIC1_CMD, ICW1_INIT);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT);
    io_wait();

    /* ICW2: vector offsets (where IRQs map in the IDT) */
    outb(PIC1_DATA, 0x20);
    io_wait(); /* Master: IRQ0 -> INT 0x20 */
    outb(PIC2_DATA, 0x28);
    io_wait(); /* Slave:  IRQ8 -> INT 0x28 */

    /* ICW3: tell master there is a slave on IRQ2, tell slave its identity */
    outb(PIC1_DATA, 0x04);
    io_wait(); /* Master: slave on IR2 (bit mask) */
    outb(PIC2_DATA, 0x02);
    io_wait(); /* Slave:  cascade identity = 2     */

    /* ICW4: both chips in 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Restore saved masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/*
 * pic_send_eoi - Signal end-of-interrupt to the PIC(s).
 * Must be called at the end of every hardware IRQ handler
 * or the PIC will never fire that IRQ again.
 */
void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI); /* Slave must be told first */
    outb(PIC1_CMD, PIC_EOI);
}

/* Mask (disable) a single IRQ line */
void pic_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t bit;

    if (irq < 8)
    {
        port = PIC1_DATA;
        bit = irq;
    }
    else
    {
        port = PIC2_DATA;
        bit = irq - 8;
    }
    outb(port, inb(port) | (1 << bit));
}

/* Unmask (enable) a single IRQ line */
void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t bit;

    if (irq < 8)
    {
        port = PIC1_DATA;
        bit = irq;
    }
    else
    {
        port = PIC2_DATA;
        bit = irq - 8;
    }
    outb(port, inb(port) & ~(1 << bit));
}

void pic_remap()
{
    outb(PIC1, 0x11);
    outb(PIC2, 0x11);

    outb(PIC1 + 1, 0x20);
    outb(PIC2 + 1, 0x28);

    outb(PIC1 + 1, 0x04);
    outb(PIC2 + 1, 0x02);

    outb(PIC1 + 1, 0x01);
    outb(PIC2 + 1, 0x01);

    outb(PIC1 + 1, 0x0);
    outb(PIC2 + 1, 0x0);
}
