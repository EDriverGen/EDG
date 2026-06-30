/* hw_spi.h -- Bare-metal STM32F103 SPI1 register-level driver for Renode testing.
 *
 * Matches the Python SPI model at evaluation/renode_tester/models/stm32_spi_hw_slave.py
 * which emulates SPI1 at 0x40013000 with two extra custom control offsets:
 *   0x80 TXN_RESET   -- writing any value resets the slave's frame state
 *                       (used to simulate CS pulse boundaries)
 *   0x84 DEV_SELECT  -- picks which slave slot the emulator routes bytes to
 *                       (for future multi-chip platforms; defaults to 0)
 *
 * Not intended for real hardware -- only for the Renode-based driver tester.
 * Generated drivers access SPI via the supplied callback/ops interface, and
 * this header provides the backing implementation.
 */
#ifndef HW_SPI_H
#define HW_SPI_H

#include <stdint.h>
#include <stddef.h>

/* ---- STM32F103 SPI1 registers (Python model base) ---- */
#define SPI1_BASE       0x40013000
#define SPI1_CR1        (*(volatile uint32_t*)(SPI1_BASE + 0x00))
#define SPI1_CR2        (*(volatile uint32_t*)(SPI1_BASE + 0x04))
#define SPI1_SR         (*(volatile uint32_t*)(SPI1_BASE + 0x08))
#define SPI1_DR         (*(volatile uint32_t*)(SPI1_BASE + 0x0C))
#define SPI1_CRCPR      (*(volatile uint32_t*)(SPI1_BASE + 0x10))

/* Custom control offsets (not on real hardware, emulator-only) */
#define SPI1_TXN_RESET  (*(volatile uint32_t*)(SPI1_BASE + 0x80))
#define SPI1_DEV_SELECT (*(volatile uint32_t*)(SPI1_BASE + 0x84))

/* RCC and GPIOA (for SPI1 pins PA5/PA6/PA7 + PA4 CS) */
#define RCC_APB2ENR     (*(volatile uint32_t*)0x40021018)
#define GPIOA_CRL       (*(volatile uint32_t*)0x40010800)
#define GPIOA_BSRR      (*(volatile uint32_t*)(0x40010800 + 0x10))

/* SR bits */
#define SPI_SR_RXNE     (1U << 0)
#define SPI_SR_TXE      (1U << 1)
#define SPI_SR_BSY      (1U << 7)

/* CR1 bits (master, 8-bit, MSB first, mode 3 defaults for ADXL345) */
#define SPI_CR1_CPHA    (1U << 0)
#define SPI_CR1_CPOL    (1U << 1)
#define SPI_CR1_MSTR    (1U << 2)
#define SPI_CR1_SPE     (1U << 6)
#define SPI_CR1_SSM     (1U << 9)
#define SPI_CR1_SSI     (1U << 8)

static inline void hw_spi1_init(void) {
    /* Clock enable: IOPAEN (bit 2), SPI1EN (bit 12) */
    RCC_APB2ENR |= (1U << 2) | (1U << 12);
    /* PA4 (CS, GPIO output 2 MHz), PA5 (SCK AF), PA7 (MOSI AF), PA6 (MISO input)
     * CRL byte layout: PA4=0x2, PA5=0xB, PA6=0x4, PA7=0xB  -> 0xB4B2_xxxx */
    uint32_t crl = GPIOA_CRL;
    crl &= ~(0xFFFF0000U);
    crl |=  (0xB4B20000U);
    GPIOA_CRL = crl;
    /* CS high (idle) via BSRR bit 4 */
    GPIOA_BSRR = (1U << 4);
    /* SPI1: master, mode 3, fPCLK/16, software NSS, 8-bit, MSB first */
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA
             | SPI_CR1_SSM  | SPI_CR1_SSI  | (3U << 3);
    SPI1_CR1 |= SPI_CR1_SPE;
}

/* CS low = BSRR bit 20 (BR4); CS high = BSRR bit 4 (BS4) */
static inline void hw_spi1_cs_lo(void) {
    GPIOA_BSRR = (1U << (20));
    /* Notify emulator of new frame start */
    SPI1_TXN_RESET = 1;
}

static inline void hw_spi1_cs_hi(void) {
    GPIOA_BSRR = (1U << 4);
}

/* Transfer one byte: write DR, wait RXNE, read DR. */
static inline uint8_t hw_spi1_xfer_byte(uint8_t tx) {
    SPI1_DR = tx;
    for (volatile int t = 0; t < 100000; t++) {
        if (SPI1_SR & SPI_SR_RXNE) break;
    }
    return (uint8_t)(SPI1_DR & 0xFFU);
}

/* Full-duplex block transfer with CS management. Passing NULL for rx
 * discards received bytes; passing NULL for tx clocks 0x00 dummy bytes. */
static inline int hw_spi1_transfer(const uint8_t *tx, uint8_t *rx, size_t len) {
    hw_spi1_cs_lo();
    for (size_t i = 0; i < len; i++) {
        uint8_t tb = tx ? tx[i] : 0x00;
        uint8_t rb = hw_spi1_xfer_byte(tb);
        if (rx) rx[i] = rb;
    }
    hw_spi1_cs_hi();
    return 0;
}

/* Callback-style interfaces matching common generated driver ops structs.
 * cs parameter is currently unused (single-CS platform) but kept for ABI
 * compatibility with drivers that expect a chip-select argument. */
static int hw_spi_write(void *ctx, uint8_t cs, const uint8_t *buf, size_t len) {
    (void)ctx; (void)cs;
    return hw_spi1_transfer(buf, NULL, len);
}

static int hw_spi_read(void *ctx, uint8_t cs, uint8_t *buf, size_t len) {
    (void)ctx; (void)cs;
    return hw_spi1_transfer(NULL, buf, len);
}

static int hw_spi_transfer(void *ctx, uint8_t cs,
                           const uint8_t *tx, uint8_t *rx, size_t len) {
    (void)ctx; (void)cs;
    return hw_spi1_transfer(tx, rx, len);
}

#endif /* HW_SPI_H */
