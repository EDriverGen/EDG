/* Functional NuttX SPI stubs — SPI_EXCHANGE/SPI_SEND/SPI_RECVBLOCK route through hw_spi.h */
#include "nuttx.h"
#include "hw_spi.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ── SPI stubs (real STM32 SPI1 via hw_spi.h) ─────────── */

void SPI_LOCK(struct spi_dev_s *dev, bool lock) { (void)dev;(void)lock; }
void SPI_SELECT(struct spi_dev_s *dev, uint32_t devid, bool selected) {
    (void)dev; (void)devid;
    if (selected) hw_spi1_cs_lo(); else hw_spi1_cs_hi();
}
uint32_t SPI_SETFREQUENCY(struct spi_dev_s *dev, uint32_t frequency) { (void)dev; return frequency; }
void SPI_SETMODE(struct spi_dev_s *dev, int mode) { (void)dev;(void)mode; }
void SPI_SETBITS(struct spi_dev_s *dev, int nbits) { (void)dev;(void)nbits; }

uint16_t SPI_SEND(struct spi_dev_s *dev, uint16_t wd) {
    (void)dev;
    return hw_spi1_xfer_byte((uint8_t)wd);
}

void SPI_EXCHANGE(struct spi_dev_s *dev, const void *txbuf, void *rxbuf, size_t nwords) {
    (void)dev;
    const uint8_t *tx = (const uint8_t *)txbuf;
    uint8_t *rx = (uint8_t *)rxbuf;
    for (size_t i = 0; i < nwords; i++) {
        uint8_t b = tx ? tx[i] : 0xFF;
        uint8_t r = hw_spi1_xfer_byte(b);
        if (rx) rx[i] = r;
    }
}

void SPI_SNDBLOCK(struct spi_dev_s *dev, const void *buf, size_t nwords) {
    (void)dev;
    const uint8_t *tx = (const uint8_t *)buf;
    for (size_t i = 0; i < nwords; i++) hw_spi1_xfer_byte(tx[i]);
}

void SPI_RECVBLOCK(struct spi_dev_s *dev, void *buf, size_t nwords) {
    (void)dev;
    uint8_t *rx = (uint8_t *)buf;
    for (size_t i = 0; i < nwords; i++) rx[i] = hw_spi1_xfer_byte(0xFF);
}


static struct i2c_master_s _i2c_inst;
int I2C_TRANSFER(struct i2c_master_s *dev, struct i2c_msg_s *msgs, int count) { (void)dev;(void)msgs;(void)count; return OK; }
int i2c_write(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR const uint8_t *b, int l) { (void)d;(void)c;(void)b;(void)l; return OK; }
int i2c_read(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR uint8_t *b, int l) { (void)d;(void)c; if(b&&l>0) memset(b,0,l); return OK; }
int i2c_writeread(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR const uint8_t *wb, int wl, FAR uint8_t *rb, int rl) { (void)d;(void)c;(void)wb;(void)wl; if(rb&&rl>0) memset(rb,0,rl); return OK; }
struct i2c_master_s *board_i2cbus_initialize(int bus) { (void)bus; return &_i2c_inst; }
int board_i2cbus_uninitialize(struct i2c_master_s *dev) { (void)dev; return OK; }

/* ── File I/O stubs ───────────────────────────────────── */
int open(const char *path, int oflag, ...) { (void)path;(void)oflag; return 3; }
int close(int fd) { (void)fd; return 0; }
int read(int fd, void *buf, size_t n) { (void)fd; if(buf&&n) memset(buf,0,n); return (int)n; }
int write(int fd, const void *buf, size_t n) { (void)fd;(void)buf; return (int)n; }
int ioctl(int fd, int req, ...) { (void)fd;(void)req; return 0; }

/* ── Timing ───────────────────────────────────────────── */
int usleep(unsigned int usec) { (void)usec; return 0; }
unsigned int sleep(unsigned int sec) { (void)sec; return 0; }
int clock_gettime(clockid_t clk_id, struct timespec *tp) { (void)clk_id; if(tp){tp->tv_sec=0;tp->tv_nsec=0;} return 0; }
void up_mdelay(unsigned int ms) { (void)ms; }
void up_udelay(unsigned int us) { (void)us; }

int printf(const char *fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) hw_uart2_putc(buf[i]);
    return n;
}
int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); int n = vsnprintf(buf, size, fmt, ap); va_end(ap); return n;
}
int syslog(int priority, const char *fmt, ...) { (void)priority;(void)fmt; return 0; }

void *kmm_malloc(size_t size) { return malloc(size); }
void *kmm_zalloc(size_t size) { return calloc(1, size); }
void  kmm_free(void *ptr) { free(ptr); }

__attribute__((weak)) int main(void) { return 0; }
