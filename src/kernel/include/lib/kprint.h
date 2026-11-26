#ifndef KPRINT_H
#define KPRINT_H

void console_putc(char c);
void kprint(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
