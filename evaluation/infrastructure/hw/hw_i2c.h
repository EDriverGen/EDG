/* hw_i2c.h — Bare-metal STM32F103 I2C1 register-level driver for Renode testing.
 * Implements the callback interface expected by generated drivers. */
#ifndef HW_I2C_H
#define HW_I2C_H

#include <stdint.h>
#include <stddef.h>

/* ---- STM32F103 I2C1 registers ---- */
#define I2C1_BASE   0x40005400
#define I2C1_CR1    (*(volatile uint32_t*)(I2C1_BASE + 0x00))
#define I2C1_CR2    (*(volatile uint32_t*)(I2C1_BASE + 0x04))
#define I2C1_OAR1   (*(volatile uint32_t*)(I2C1_BASE + 0x08))
#define I2C1_DR     (*(volatile uint32_t*)(I2C1_BASE + 0x10))
#define I2C1_SR1    (*(volatile uint32_t*)(I2C1_BASE + 0x14))
#define I2C1_SR2    (*(volatile uint32_t*)(I2C1_BASE + 0x18))
#define I2C1_CCR    (*(volatile uint32_t*)(I2C1_BASE + 0x1C))
#define I2C1_TRISE  (*(volatile uint32_t*)(I2C1_BASE + 0x20))

/* RCC */
#define RCC_APB1ENR (*(volatile uint32_t*)0x4002101C)
#define RCC_APB2ENR (*(volatile uint32_t*)0x40021018)

/* GPIOB for I2C1: PB6=SCL, PB7=SDA */
#define GPIOB_CRL   (*(volatile uint32_t*)0x40010C00)

static inline void hw_i2c1_init(void) {
    RCC_APB2ENR |= (1 << 3);           /* IOPBEN */
    RCC_APB1ENR |= (1 << 21);          /* I2C1EN */
    /* PB6, PB7: AF open-drain, 2 MHz  (CNF=11, MODE=10 → 0xE) */
    uint32_t crl = GPIOB_CRL;
    crl &= ~(0xFFU << 24);
    crl |=  (0xEEU << 24);
    GPIOB_CRL = crl;
    /* Reset I2C */
    I2C1_CR1 = (1 << 15);              /* SWRST */
    I2C1_CR1 = 0;
    I2C1_CR2 = 8;                      /* FREQ = 8 MHz */
    I2C1_CCR = 40;                     /* 100 kHz SCL */
    I2C1_TRISE = 9;
    I2C1_CR1 = (1 << 0);               /* PE */
}

static inline int hw_i2c1_start(void) {
    I2C1_CR1 |= (1 << 10) | (1 << 8);  /* ACK | START */
    for (volatile int t = 0; t < 100000; t++)
        if (I2C1_SR1 & (1 << 0)) return 0;  /* SB */
    return -1;
}

static inline int hw_i2c1_send_addr(uint8_t addr_rw) {
    I2C1_DR = addr_rw;
    for (volatile int t = 0; t < 100000; t++)
        if (I2C1_SR1 & (1 << 1)) { (void)I2C1_SR2; return 0; }  /* ADDR */
    return -1;
}

static inline int hw_i2c1_send_byte(uint8_t b) {
    I2C1_DR = b;
    for (volatile int t = 0; t < 100000; t++)
        if (I2C1_SR1 & (1 << 7)) return 0;  /* TXE */
    return -1;
}

static inline int hw_i2c1_recv_byte(uint8_t *b, int last) {
    if (last) I2C1_CR1 &= ~(1 << 10);  /* NACK for last byte */
    for (volatile int t = 0; t < 100000; t++)
        if (I2C1_SR1 & (1 << 6)) { *b = (uint8_t)I2C1_DR; return 0; }  /* RXNE */
    return -1;
}

static inline void hw_i2c1_stop(void) {
    I2C1_CR1 |= (1 << 9);  /* STOP */
}

/* ---- Callback functions matching common generated I2C adapter shapes ---- */

/* write: START -> ADDR+W -> data[0..len-1] -> STOP */
static int hw_i2c_write(void *ctx, uint8_t addr, const uint8_t *data, size_t len) {
    (void)ctx;
    if (hw_i2c1_start()) return -1;
    if (hw_i2c1_send_addr(addr << 1)) return -1;
    for (size_t i = 0; i < len; i++)
        if (hw_i2c1_send_byte(data[i])) return -1;
    hw_i2c1_stop();
    return 0;
}

/* read: START -> ADDR+R -> read data[0..len-1] -> STOP */
static int hw_i2c_read(void *ctx, uint8_t addr, uint8_t *data, size_t len) {
    (void)ctx;
    if (hw_i2c1_start()) return -1;
    if (hw_i2c1_send_addr((addr << 1) | 1)) return -1;
    for (size_t i = 0; i < len; i++)
        if (hw_i2c1_recv_byte(&data[i], (i == len - 1))) return -1;
    hw_i2c1_stop();
    return 0;
}

/* write_read: write phase then repeated-start read phase */
static int hw_i2c_write_read(void *ctx, uint8_t addr,
                              const uint8_t *wdata, size_t wlen,
                              uint8_t *rdata, size_t rlen) {
    (void)ctx;
    if (hw_i2c1_start()) return -1;
    if (hw_i2c1_send_addr(addr << 1)) return -1;
    for (size_t i = 0; i < wlen; i++)
        if (hw_i2c1_send_byte(wdata[i])) return -1;
    /* Repeated start */
    if (hw_i2c1_start()) return -1;
    if (hw_i2c1_send_addr((addr << 1) | 1)) return -1;
    for (size_t i = 0; i < rlen; i++)
        if (hw_i2c1_recv_byte(&rdata[i], (i == rlen - 1))) return -1;
    hw_i2c1_stop();
    return 0;
}

#endif /* HW_I2C_H */
