#include "keyboard.h"
#include "../../arch/x86/io.h" /* inb, outb already defined here */
#include "../../log/log.h"
#include "../../gdt_and_idt/idt.h"
#include "../../drivers/vga/vga.h"

#define KEYBOARD_DATA_PORT 0x60
#define PIC_MASTER_CMD 0x20
#define PIC_EOI 0x20

/* US QWERTY scancode set 1 — index = scancode, value = ASCII */
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '};

#define SCANCODE_TABLE_SIZE ((int)(sizeof(scancode_to_ascii) / sizeof(scancode_to_ascii[0])))

static void keyboard_handler(interrupt_frame_t *frame)
{
    (void)frame;
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* Bit 7 set = key release, ignore it */
    if (scancode & 0x80)
    {
        outb(PIC_MASTER_CMD, PIC_EOI);
        return;
    }

    /* Print the character if we have a mapping */
    if (scancode < SCANCODE_TABLE_SIZE && scancode_to_ascii[scancode])
    {
        char c = scancode_to_ascii[scancode];
        log_info("Key: '%c' (scancode=0x%x)", c, scancode);
        vga_put_char(c); /* echo the character directly to screen */
    }
    else
    {
        log_info("Key: (special, scancode=0x%x)", scancode);
    }

    /* Send EOI — MUST do this or PIC blocks all future keyboard IRQs */
    outb(PIC_MASTER_CMD, PIC_EOI);
}

void keyboard_init(void)
{
    idt_register_handler(33, keyboard_handler);
    log_info("Keyboard driver initialized.");
}