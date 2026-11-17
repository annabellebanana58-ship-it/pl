#include "uart.h"

#define UART0_BASE  0x3F201000UL

#define UART0_DR    (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART0_FR    (*(volatile unsigned int *)(UART0_BASE + 0x18))
#define UART0_IBRD  (*(volatile unsigned int *)(UART0_BASE + 0x24))
#define UART0_FBRD  (*(volatile unsigned int *)(UART0_BASE + 0x28))
#define UART0_LCRH  (*(volatile unsigned int *)(UART0_BASE + 0x2C))
#define UART0_CR    (*(volatile unsigned int *)(UART0_BASE + 0x30))
#define UART0_ICR   (*(volatile unsigned int *)(UART0_BASE + 0x44))

#define UART_FR_TXFF (1U << 5)
#define UART_LCRH_WLEN_8 (3U << 5)
#define UART_LCRH_FEN    (1U << 4)
#define UART_CR_UARTEN   (1U << 0)
#define UART_CR_TXE      (1U << 8)
#define UART_CR_RXE      (1U << 9)

void
uart_init(void)
{
    UART0_CR = 0;
    UART0_ICR = 0x7FF;

    UART0_IBRD = 26;
    UART0_FBRD = 3;

    UART0_LCRH = UART_LCRH_WLEN_8 | UART_LCRH_FEN;
    UART0_CR = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
}

void
uart_putc(char c)
{
    while (UART0_FR & UART_FR_TXFF) {
    }
    UART0_DR = (unsigned int)c;
}
