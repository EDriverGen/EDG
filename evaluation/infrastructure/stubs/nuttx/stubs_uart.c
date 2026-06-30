/* Functional NuttX UART stubs — write()/read() route through hw_uart_bus.h
 *
 * NuttX drivers open /dev/ttyS0 and use POSIX write/read.
 * This stub makes those functions drive real USART1 via hw_uart_bus.h.
 */
#include "nuttx.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/* ── File I/O stubs (UART-functional) ─────────────────── */

int open(const char *path, int oflag, ...) {
    (void)path; (void)oflag;
    hw_uart_bus_init();
    return 3;  /* fake fd */
}

int close(int fd) { (void)fd; return 0; }

int write(int fd, const void *buf, size_t n) {
    (void)fd;
    const uint8_t *p = (const uint8_t *)buf;
    for (size_t i = 0; i < n; i++)
        hw_uart_bus_write_byte(p[i]);
    return (int)n;
}

int read(int fd, void *buf, size_t n) {
    (void)fd;
    uint8_t *p = (uint8_t *)buf;
    for (size_t i = 0; i < n; i++) {
        if (hw_uart_bus_read_byte(&p[i]) != 0)
            return (int)i;
    }
    return (int)n;
}

int ioctl(int fd, int req, ...) { (void)fd; (void)req; return 0; }

/* ── I2C dummies ──────────────────────────────────────── */
static struct i2c_master_s _i2c_inst;
int I2C_TRANSFER(struct i2c_master_s *dev, struct i2c_msg_s *msgs, int count) { (void)dev;(void)msgs;(void)count; return OK; }
int i2c_write(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR const uint8_t *b, int l) { (void)d;(void)c;(void)b;(void)l; return OK; }
int i2c_read(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR uint8_t *b, int l) { (void)d;(void)c; if(b&&l>0) memset(b,0,l); return OK; }
int i2c_writeread(FAR struct i2c_master_s *d, FAR const struct i2c_config_s *c, FAR const uint8_t *wb, int wl, FAR uint8_t *rb, int rl) { (void)d;(void)c;(void)wb;(void)wl; if(rb&&rl>0) memset(rb,0,rl); return OK; }
struct i2c_master_s *board_i2cbus_initialize(int bus) { (void)bus; return &_i2c_inst; }
int board_i2cbus_uninitialize(struct i2c_master_s *dev) { (void)dev; return OK; }

/* ── SPI dummies ──────────────────────────────────────── */
void SPI_LOCK(struct spi_dev_s *dev, bool lock) { (void)dev;(void)lock; }
void SPI_SELECT(struct spi_dev_s *dev, uint32_t devid, bool selected) { (void)dev;(void)devid;(void)selected; }
uint32_t SPI_SETFREQUENCY(struct spi_dev_s *dev, uint32_t f) { (void)dev; return f; }
void SPI_SETMODE(struct spi_dev_s *dev, int m) { (void)dev;(void)m; }
void SPI_SETBITS(struct spi_dev_s *dev, int n) { (void)dev;(void)n; }
uint16_t SPI_SEND(struct spi_dev_s *dev, uint16_t wd) { (void)dev;(void)wd; return 0; }
void SPI_EXCHANGE(struct spi_dev_s *dev, const void *tx, void *rx, size_t n) { (void)dev;(void)tx; if(rx&&n) memset(rx,0,n); }
void SPI_SNDBLOCK(struct spi_dev_s *dev, const void *buf, size_t n) { (void)dev;(void)buf;(void)n; }
void SPI_RECVBLOCK(struct spi_dev_s *dev, void *buf, size_t n) { (void)dev; if(buf&&n) memset(buf,0,n); }

/* ── Timing ───────────────────────────────────────────── */
int usleep(unsigned int usec) { (void)usec; return 0; }
unsigned int sleep(unsigned int sec) { (void)sec; return 0; }
int clock_gettime(clockid_t clk_id, struct timespec *tp) { (void)clk_id; if(tp){tp->tv_sec=0;tp->tv_nsec=0;} return 0; }
void up_mdelay(unsigned int ms) { (void)ms; }
void up_udelay(unsigned int us) { (void)us; }

/* ── POSIX termios stubs ──────────────────────────────── */
typedef unsigned int speed_t;
struct termios { int dummy; };
int tcgetattr(int fd, struct termios *t) { (void)fd;(void)t; return 0; }
int tcsetattr(int fd, int act, const struct termios *t) { (void)fd;(void)act;(void)t; return 0; }
speed_t cfsetispeed(struct termios *t, speed_t s) { (void)t;(void)s; return 0; }
speed_t cfsetospeed(struct termios *t, speed_t s) { (void)t;(void)s; return 0; }

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
