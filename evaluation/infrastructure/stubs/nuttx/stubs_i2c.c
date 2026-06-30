/* Functional NuttX I2C stubs — I2C_TRANSFER/i2c_write/i2c_read route through hw_i2c.h */
#include "nuttx.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ── I2C stubs (real STM32 I2C1 via hw_i2c.h) ─────────── */

static struct i2c_master_s _i2c_inst;

int I2C_TRANSFER(struct i2c_master_s *dev, struct i2c_msg_s *msgs, int count) {
    (void)dev;
    for (int i = 0; i < count; i++) {
        if (msgs[i].flags & I2C_M_READ) {
            hw_i2c_read(0, (uint8_t)msgs[i].addr, msgs[i].buffer, (uint16_t)msgs[i].length);
        } else {
            hw_i2c_write(0, (uint8_t)msgs[i].addr, msgs[i].buffer, (uint16_t)msgs[i].length);
        }
    }
    return OK;
}

int i2c_write(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
              FAR const uint8_t *buffer, int buflen) {
    (void)dev;
    return hw_i2c_write(0, (uint8_t)config->address, buffer, (uint16_t)buflen) == 0 ? OK : -1;
}

int i2c_read(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
             FAR uint8_t *buffer, int buflen) {
    (void)dev;
    return hw_i2c_read(0, (uint8_t)config->address, buffer, (uint16_t)buflen) == 0 ? OK : -1;
}

int i2c_writeread(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
                  FAR const uint8_t *wbuf, int wlen,
                  FAR uint8_t *rbuf, int rlen) {
    (void)dev;
    return hw_i2c_write_read(0, (uint8_t)config->address, wbuf, (uint16_t)wlen,
                             rbuf, (uint16_t)rlen) == 0 ? OK : -1;
}

struct i2c_master_s *board_i2cbus_initialize(int bus) {
    (void)bus; hw_i2c1_init(); return &_i2c_inst;
}
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
int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    (void)clk_id; if(tp){ tp->tv_sec=0; tp->tv_nsec=0; } return 0;
}
void up_mdelay(unsigned int ms) { (void)ms; }
void up_udelay(unsigned int us) { (void)us; }

/* ── NuttX SPI dummies ────────────────────────────────── */
void SPI_LOCK(struct spi_dev_s *d, bool l) { (void)d;(void)l; }
void SPI_SELECT(struct spi_dev_s *d, uint32_t id, bool s) { (void)d;(void)id;(void)s; }
uint32_t SPI_SETFREQUENCY(struct spi_dev_s *d, uint32_t f) { (void)d; return f; }
void SPI_SETMODE(struct spi_dev_s *d, int m) { (void)d;(void)m; }
void SPI_SETBITS(struct spi_dev_s *d, int b) { (void)d;(void)b; }
uint16_t SPI_SEND(struct spi_dev_s *d, uint16_t w) { (void)d;(void)w; return 0; }
void SPI_EXCHANGE(struct spi_dev_s *d, const void *t, void *r, size_t n) { (void)d;(void)t; if(r&&n) memset(r,0,n); }
void SPI_SNDBLOCK(struct spi_dev_s *d, const void *b, size_t n) { (void)d;(void)b;(void)n; }
void SPI_RECVBLOCK(struct spi_dev_s *d, void *b, size_t n) { (void)d; if(b&&n) memset(b,0,n); }

/* ── printf/syslog ────────────────────────────────────── */
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

/* ── Memory ───────────────────────────────────────────── */
void *kmm_malloc(size_t size) { return malloc(size); }
void *kmm_zalloc(size_t size) { return calloc(1, size); }
void  kmm_free(void *ptr) { free(ptr); }

__attribute__((weak)) int main(void) { return 0; }
