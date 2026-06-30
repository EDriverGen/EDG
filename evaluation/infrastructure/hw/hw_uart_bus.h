/* hw_uart_bus.h -- Bare-metal STM32F103 USART1 helper for UART-bus tests.
 *
 * Matches the Python model at evaluation/renode_tester/models/stm32_usart_hw_slave.py
 * which treats USART1 @ 0x40013800 as a command/response oracle: the MCU
 * transmits fixed-length request frames and receives response bytes from a
 * per-vector table. This header provides raw TX/RX primitives plus a
 * blocking transaction helper.
 *
 * Not for real hardware: Renode-only. Debug output still routes through
 * USART2 via the sibling hw_uart.h.
 */
#ifndef HW_UART_BUS_H
#define HW_UART_BUS_H

#include <stdint.h>
#include <stddef.h>

/* USART1 registers (Python model base) */
#define USART1_BASE  0x40013800
#define USART1_SR    (*(volatile uint32_t*)(USART1_BASE + 0x00))
#define USART1_DR    (*(volatile uint32_t*)(USART1_BASE + 0x04))
#define USART1_BRR   (*(volatile uint32_t*)(USART1_BASE + 0x08))
#define USART1_CR1   (*(volatile uint32_t*)(USART1_BASE + 0x0C))

/* RCC APB2ENR: bit 14 = USART1EN, bit 2 = IOPAEN */
#define RCC_APB2ENR  (*(volatile uint32_t*)0x40021018)

/* GPIOA CRH for PA9 (TX AF PP 2MHz = 0xA) and PA10 (RX floating input = 0x4) */
#define GPIOA_CRH    (*(volatile uint32_t*)0x40010804)

/* SR bits (must match Python model) */
#define USART_SR_RXNE  (1U << 5)
#define USART_SR_TC    (1U << 6)
#define USART_SR_TXE   (1U << 7)

/* CR1 bits */
#define USART_CR1_RE   (1U << 2)
#define USART_CR1_TE   (1U << 3)
#define USART_CR1_UE   (1U << 13)

static inline void hw_uart_bus_init(void) {
    /* Clock: USART1 + GPIOA */
    RCC_APB2ENR |= (1U << 14) | (1U << 2);
    /* PA9 TX (AF PP 2MHz = 0xA), PA10 RX (floating input = 0x4) */
    uint32_t crh = GPIOA_CRH;
    crh &= ~(0xFFU << 4);
    crh |=  (0x4AU << 4);
    GPIOA_CRH = crh;
    /* Baud = 9600 @ fck=8MHz: BRR = 8_000_000/9600 ~= 833 = 0x0341. The
     * Python model ignores BRR but we still write it for realism. */
    USART1_BRR = 0x0341;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static inline void hw_uart_bus_write_byte(uint8_t b) {
    for (volatile int t = 0; t < 100000; t++) {
        if (USART1_SR & USART_SR_TXE) break;
    }
    USART1_DR = b;
}

static inline int hw_uart_bus_read_byte(uint8_t *b) {
    for (volatile int t = 0; t < 200000; t++) {
        if (USART1_SR & USART_SR_RXNE) {
            *b = (uint8_t)(USART1_DR & 0xFFU);
            return 0;
        }
    }
    return -1;  /* timeout */
}

/* Blocking fixed-length transaction: TX request, then RX response. */
static inline int hw_uart_bus_txrx(const uint8_t *cmd, uint8_t *resp,
                                   size_t packet_len) {
    for (size_t i = 0; i < packet_len; i++) {
        hw_uart_bus_write_byte(cmd[i]);
    }
    for (size_t i = 0; i < packet_len; i++) {
        if (hw_uart_bus_read_byte(&resp[i]) != 0) return -1;
    }
    return 0;
}

/* Callback adapter matching common generated driver op struct shapes. */
static int hw_uart_transfer(void *ctx, const uint8_t *tx, size_t tx_len,
                            uint8_t *rx, size_t rx_len) {
    (void)ctx;
    if (tx && tx_len) {
        for (size_t i = 0; i < tx_len; i++) hw_uart_bus_write_byte(tx[i]);
    }
    if (rx && rx_len) {
        for (size_t i = 0; i < rx_len; i++) {
            if (hw_uart_bus_read_byte(&rx[i]) != 0) return -1;
        }
    }
    return 0;
}

#endif /* HW_UART_BUS_H */
