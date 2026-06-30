/*
 * NuttX unified API stub for syntax-only compilation tests.
 * Covers: I2C master (struct i2c_master_s, i2c_write/read, I2C_TRANSFER),
 *         GPIO character-device (open, ioctl, GPIOC_WRITE/READ),
 *         POSIX-like file I/O, timing (usleep, sleep, clock).
 */
#ifndef __NUTTX_STUB_H__
#define __NUTTX_STUB_H__

/* Block ARM newlib's own struct timespec / clockid_t before including system headers */
#define _SYS__TIMESPEC_H_
#define _SYS_TIMESPEC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NuttX attribute macros */
#ifndef FAR
#define FAR
#endif
#ifndef NEAR
#define NEAR
#endif

/* ---------- errno & return codes ---------- */
#ifndef OK
#define OK 0
#endif
#ifndef ERROR
#define ERROR (-1)
#endif

#ifndef ENOMEM
#define ENOMEM 12
#endif
#ifndef EINVAL
#define EINVAL 22
#endif
#ifndef EIO
#define EIO 5
#endif
#ifndef ENODEV
#define ENODEV 19
#endif

/* ---------- File I/O ---------- */
int open(const char *path, int oflag, ...);
int close(int fd);
int read(int fd, void *buf, size_t nbytes);
int write(int fd, const void *buf, size_t nbytes);
int ioctl(int fd, int req, ...);

#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_NONBLOCK  0x4000
#define O_NOCTTY    0x0100
#define TCIOFLUSH   2
static inline int tcflush(int fd, int q) { (void)fd; (void)q; return 0; }

/* ---------- Timing ---------- */
int usleep(unsigned int usec);
unsigned int sleep(unsigned int sec);

/* clock_gettime support */
#ifndef __clockid_t_defined
typedef int clockid_t;
#define __clockid_t_defined
#endif
#define CLOCK_MONOTONIC 1
#define CLOCK_REALTIME  0
struct timespec {
    long tv_sec;
    long tv_nsec;
};
int clock_gettime(clockid_t clk_id, struct timespec *tp);

/* ---------- printf ---------- */
int printf(const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int syslog(int priority, const char *fmt, ...);
#define LOG_INFO  6
#define LOG_ERR   3
#define LOG_WARNING 4

/* ---------- I2C master ---------- */
struct i2c_msg_s {
    uint32_t frequency;
    uint16_t addr;
    uint16_t flags;
    uint8_t *buffer;
    int      length;
};

struct i2c_config_s {
    uint32_t frequency;
    uint16_t address;
    uint8_t  addrlen;   /* 7 or 10 */
};

#define I2C_M_READ      0x0001
#define I2C_M_TEN       0x0010
#define I2C_M_NOSTOP    0x0040
#define I2C_M_NOSTART   0x0080

struct i2c_master_s {
    /* opaque lower-half driver */
    void *priv;
};

/* Transfer function */
int I2C_TRANSFER(struct i2c_master_s *dev, struct i2c_msg_s *msgs, int count);

/* Convenience helpers (config-based API used by NuttX reference drivers) */
int i2c_write(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
              FAR const uint8_t *buffer, int buflen);
int i2c_read(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
             FAR uint8_t *buffer, int buflen);
int i2c_writeread(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
                  FAR const uint8_t *wbuf, int wlen,
                  FAR uint8_t *rbuf, int rlen);

/* Board-level I2C bus initialization */
struct i2c_master_s *board_i2cbus_initialize(int bus);
int board_i2cbus_uninitialize(struct i2c_master_s *dev);

/* I2C frequency setting */
#define I2C_SPEED_STANDARD    100000
#define I2C_SPEED_FAST        400000
#define I2C_SPEED_FAST_PLUS  1000000

/* ---------- GPIO character-device ---------- */
/* ioctl commands for /dev/gpioN */
#define GPIOC_WRITE     0x0001
#define GPIOC_READ      0x0002
#define GPIOC_SETPINTYPE 0x0003
#define GPIOC_REGISTER  0x0004
#define GPIOC_UNREGISTER 0x0005

/* GPIO pin types */
#define GPIO_INPUT_PIN          0
#define GPIO_INPUT_PIN_PULLUP   1
#define GPIO_INPUT_PIN_PULLDOWN 2
#define GPIO_OUTPUT_PIN         3
#define GPIO_OUTPUT_PIN_OPENDRAIN 4
#define GPIO_INTERRUPT_PIN      5
#define GPIO_INTERRUPT_RISING_PIN  6
#define GPIO_INTERRUPT_FALLING_PIN 7
#define GPIO_INTERRUPT_BOTH_PIN    8

/* ---------- Memory allocation ---------- */
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);
void *kmm_malloc(size_t size);
void *kmm_zalloc(size_t size);
void  kmm_free(void *ptr);

/* ---------- Misc ---------- */
void up_mdelay(unsigned int ms);
void up_udelay(unsigned int us);
#define ASSERT(x) ((void)(x))
#define DEBUGASSERT(x) ((void)(x))
#define UNUSED(x) ((void)(x))

#ifdef __cplusplus
}
#endif

#endif /* __NUTTX_STUB_H__ */
