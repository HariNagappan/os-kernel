#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void vga_init();
void vga_clear();
void vga_put_char(char c);
void vga_write(const char* str);
void vga_set_color(uint8_t color);

#endif