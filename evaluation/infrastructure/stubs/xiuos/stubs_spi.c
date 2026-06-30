/* Functional XiUOS SPI stubs — PrivIoctl(SPI_IOC_TRANSFER) / SpiDataParam route through hw_spi.h
 *
 * XiUOS drivers open a SPI bus device, then use PrivIoctl with spi_ioc_transfer
 * or SpiDataParam to perform SPI transactions.
 */
#include "xiuos.h"
#include "hw_spi.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ── Transform layer stubs (with SPI routing) ─────────── */

int PrivOpen(const char *path, int oflag) {
    (void)path; (void)oflag;
    hw_spi1_init();
    return 3;
}

int PrivClose(int fd) { (void)fd; return 0; }

int PrivRead(int fd, void *buf, size_t n) {
    (void)fd;
    uint8_t *rx = (uint8_t *)buf;
    hw_spi1_cs_lo();
    for (size_t i = 0; i < n; i++)
        rx[i] = hw_spi1_xfer_byte(0xFF);
    hw_spi1_cs_hi();
    return (int)n;
}

int PrivWrite(int fd, const void *buf, size_t n) {
    (void)fd;
    const uint8_t *tx = (const uint8_t *)buf;
    hw_spi1_cs_lo();
    for (size_t i = 0; i < n; i++)
        hw_spi1_xfer_byte(tx[i]);
    hw_spi1_cs_hi();
    return (int)n;
}

int PrivIoctl(int fd, int cmd, void *arg) {
    (void)fd;
    if (cmd == SPI_IOC_TRANSFER && arg) {
        struct spi_ioc_transfer *xfer = (struct spi_ioc_transfer *)arg;
        const uint8_t *tx = (const uint8_t *)xfer->tx_buf;
        uint8_t *rx = (uint8_t *)xfer->rx_buf;
        hw_spi1_cs_lo();
        for (uint32_t i = 0; i < xfer->len; i++) {
            uint8_t b = tx ? tx[i] : 0xFF;
            uint8_t r = hw_spi1_xfer_byte(b);
            if (rx) rx[i] = r;
        }
        hw_spi1_cs_hi();
        return 0;
    }
    /* Also handle PrivIoctlCfg wrapper pattern */
    if (arg) {
        PrivIoctlCfg *cfg = (PrivIoctlCfg *)arg;
        if (cfg->ioctl_driver_type == SPI_IOC_TRANSFER && cfg->args) {
            SpiDataParam *sp = (SpiDataParam *)cfg->args;
            const uint8_t *tx = (const uint8_t *)sp->tx_buff;
            uint8_t *rx = (uint8_t *)sp->rx_buff;
            hw_spi1_cs_lo();
            for (uint32_t i = 0; i < sp->length; i++) {
                uint8_t b = tx ? tx[i] : 0xFF;
                uint8_t r = hw_spi1_xfer_byte(b);
                if (rx) rx[i] = r;
            }
            hw_spi1_cs_hi();
            return 0;
        }
    }
    return 0;
}

void PrivTaskDelay(int32_t ms) { (void)ms; }

/* ── Mutex / Semaphore ────────────────────────────────── */
int PrivMutexCreate(void **m, int a) { (void)m;(void)a; return 0; }
int PrivMutexDelete(void *m) { (void)m; return 0; }
int PrivMutexObtain(void *m) { (void)m; return 0; }
int PrivMutexAbandon(void *m) { (void)m; return 0; }
int PrivSemaphoreCreate(void **s, int a, int c) { (void)s;(void)a;(void)c; return 0; }
int PrivSemaphoreDelete(void *s) { (void)s; return 0; }
int PrivSemaphoreObtainWait(void *s, int32_t ms) { (void)s;(void)ms; return 0; }
int PrivSemaphoreAbandon(void *s) { (void)s; return 0; }

/* ── Bus framework (SPI) ─────────────────────────────── */
BusType BusFind(const char *name) { (void)name; return (BusType)1; }
HardwareDevType BusFindDevice(BusType bus, const char *name) { (void)bus;(void)name; return (HardwareDevType)1; }
int BusDevOpen(HardwareDevType dev) { (void)dev; hw_spi1_init(); return 0; }
int BusDevClose(HardwareDevType dev) { (void)dev; return 0; }
int BusDevWriteData(HardwareDevType dev, void *wp) { (void)dev;(void)wp; return 0; }
int BusDevReadData(HardwareDevType dev, void *rp) { (void)dev;(void)rp; return 0; }
int BusDrvConfigure(HardwareDevType dev, void *cfg) { (void)dev;(void)cfg; return 0; }

/* ── Memory ───────────────────────────────────────────── */
void *PrivMalloc(size_t sz) { return malloc(sz); }
void *PrivCalloc(size_t n, size_t sz) { return calloc(n, sz); }
void PrivFree(void *p) { free(p); }

/* ── printf via UART2 ─────────────────────────────────── */
int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
