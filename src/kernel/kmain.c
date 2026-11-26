#include "kmain.h"
#include "uart.h"
#include "kprint.h"
#include "exceptions.h"

void
kmain(void)
{
    uart_init();
    kprint("booted!!!, kmain at %p\n", (void *)kmain);

    /* temporary exception test */
    __asm__ volatile("udf #0");

    for (;;)
        __asm__ volatile("wfe"); /* now we wait... */
}
