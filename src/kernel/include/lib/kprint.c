#include <stdint.h>
#include <stdarg.h>
#include "kprint.h"
#include "uart.h"

void
console_putc(char c)
{
    if (c == '\n')
        uart_putc('\r');
    uart_putc(c);
}

static void
put_uint(unsigned long long v, unsigned base)
{
    char buf[20];
    int i = 0;

    do {
        buf[i++] = "0123456789abcdef"[v % base];
        v /= base;
    } while (v);
    while (i--)
        console_putc(buf[i]);
}

void
kprint(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            console_putc(*fmt);
            continue;
        }

        fmt++;

        uint8_t longbool = 0;

        while (*fmt == 'l') {
                longbool = 1;
                fmt++;
        }

        switch (*fmt) {
            case 'c':
                console_putc((char)va_arg(ap, int));
                break;
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s)
                    s = "(null)";
                while (*s)
                    console_putc(*s++);
                break;
            }
            case 'd': {
                long long v;
                unsigned long long mag;

                if (longbool) {
                    v = va_arg(ap, long long);
                } else {
                    v = va_arg(ap, int);
                }

                if (v < 0) {
                    console_putc('-');
                    mag = 0ULL - (unsigned long long)v;
                } else {
                    mag = (unsigned long long)v;
                }

                put_uint(mag, 10);
                break;
            }
            case 'u':
                if (longbool == 1) {
                    put_uint(va_arg(ap, unsigned long long), 10);
                } else {
                    put_uint(va_arg(ap, unsigned int), 10);
                }
                break;
            case 'x':
                if (longbool == 1) {
                    put_uint(va_arg(ap, unsigned long long), 16);
                } else {
                    put_uint(va_arg(ap, unsigned int), 16);
                }
                break;
            case 'p':
                console_putc('0');
                console_putc('x');
                put_uint((uintptr_t)va_arg(ap, void *), 16);
                break;
            case '%':
                console_putc('%');
                break;
            case '\0':
                va_end(ap);
                return;
            default:
                console_putc('%');
                console_putc(*fmt);
                break;
        }
    }
    va_end(ap);
}
