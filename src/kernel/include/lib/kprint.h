#ifndef KPRINT_H
#define KPRINT_H

void console_putc(char c);
static void put_uint(unsigned long long v, unsigned base);
void kprint(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
