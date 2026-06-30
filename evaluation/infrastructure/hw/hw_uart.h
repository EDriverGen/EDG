/* hw_uart.h — USART2 output for Renode test harness. */
#ifndef HW_UART_H
#define HW_UART_H

#include <stdint.h>

/* STM32F103 USART2 @ APB1 */
#define USART2_SR   (*(volatile uint32_t*)0x40004400)
#define USART2_DR   (*(volatile uint32_t*)0x40004404)
#define USART2_BRR  (*(volatile uint32_t*)0x40004408)
#define USART2_CR1  (*(volatile uint32_t*)0x4000440C)

static inline void hw_uart2_init(void) {
    /* Enable GPIOA + USART2 clocks */
    (*(volatile uint32_t*)0x40021018) |= (1 << 2);   /* RCC_APB2ENR IOPAEN */
    (*(volatile uint32_t*)0x4002101C) |= (1 << 17);  /* RCC_APB1ENR USART2EN */
    /* PA2 = USART2_TX: AF push-pull 2 MHz (CNF=10, MODE=10 → 0xA) */
    volatile uint32_t *gpioa_crl = (volatile uint32_t*)0x40010800;
    uint32_t v = *gpioa_crl;
    v &= ~(0xFU << 8);
    v |=  (0xAU << 8);
    *gpioa_crl = v;
    USART2_BRR = 0x0045;  /* ~115200 @ 8MHz */
    USART2_CR1 = (1 << 13) | (1 << 3);  /* UE | TE */
}

static inline void hw_uart2_putc(char c) {
    while (!(USART2_SR & (1 << 7))) {}
    USART2_DR = (uint8_t)c;
}

static inline void hw_uart2_puts(const char *s) {
    while (*s) hw_uart2_putc(*s++);
}

/* Simple integer to decimal */
static inline void hw_uart2_print_int(int32_t v) {
    char buf[12];
    int i = 0;
    if (v < 0) { hw_uart2_putc('-'); v = -v; }
    if (v == 0) { hw_uart2_putc('0'); return; }
    while (v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) hw_uart2_putc(buf[i]);
}

static inline void hw_uart2_print_hex8(uint8_t v) {
    const char hex[] = "0123456789ABCDEF";
    hw_uart2_puts("0x");
    hw_uart2_putc(hex[v >> 4]);
    hw_uart2_putc(hex[v & 0xF]);
}

#endif /* HW_UART_H */
