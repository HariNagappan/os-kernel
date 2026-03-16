#include "printf.h"
#include "vga.h"
#include <stdarg.h>

static void print_number(int num)
{
    char buf[32];
    int i = 0;

    if (num == 0)
    {
        vga_put_char('0');
        return;
    }

    if (num < 0)
    {
        vga_put_char('-');
        num = -num;
    }

    while (num > 0)
    {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }

    for (int j = i - 1; j >= 0; j--)
        vga_put_char(buf[j]);
}

static void print_hex(unsigned int num)
{
    char hex[] = "0123456789ABCDEF";
    int started = 0;

    for (int i = 28; i >= 0; i -= 4)
    {
        int digit = (num >> i) & 0xF;

        if (digit != 0 || started || i == 0)
        {
            started = 1;
            vga_put_char(hex[digit]);
        }
    }
}

void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt)
    {
        if (*fmt == '%')
        {
            fmt++;

            switch (*fmt)
            {
                case 'd': print_number(va_arg(args, int)); break;
                case 'x': print_hex(va_arg(args, unsigned int)); break;
                case 's': vga_write(va_arg(args, char*)); break;
                case 'c': vga_put_char((char)va_arg(args, int)); break;
                default: vga_put_char(*fmt);
            }
        }
        else
        {
            vga_put_char(*fmt);
        }

        fmt++;
    }

    va_end(args);
}