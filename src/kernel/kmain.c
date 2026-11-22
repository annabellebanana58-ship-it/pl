#include "uart.h"
#include "kprint.h"

void
kmain(void)
{
    uart_init();
    kprint("booted!!!, kmain at %p\n", (void *)kmain);

    for (;;)
        __asm__ volatile("wfe"); /* now we wait... */
}
