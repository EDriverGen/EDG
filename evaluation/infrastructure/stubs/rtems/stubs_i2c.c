/* Verified against data/rtos/rtems/cpukit/dev/i2c/i2c-bus.c on 2026-05-12. */
#include "rtems.h"
#include "dev/i2c/i2c.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int g_next_fd = 3;
static rtems_interval g_ticks;
static uint8_t g_i2c_default_addr = 0x23;

int open(const char *path, int oflag, ...) {
    (void)path; (void)oflag;
    hw_i2c1_init();
    return g_next_fd++;
}

int close(int fd) {
    (void)fd;
    return 0;
}

ssize_t read(int fd, void *buf, size_t nbyte) {
    (void)fd;
    return hw_i2c_read(0, g_i2c_default_addr, buf, nbyte) == 0 ? (ssize_t)nbyte : -1;
}

ssize_t write(int fd, const void *buf, size_t nbyte) {
    (void)fd;
    return hw_i2c_write(0, g_i2c_default_addr, buf, nbyte) == 0 ? (ssize_t)nbyte : -1;
}

int ioctl(int fd, unsigned long request, ...) {
    (void)fd;
    va_list ap;
    va_start(ap, request);
    void *arg = va_arg(ap, void *);
    va_end(ap);

    if (request == I2C_SLAVE || request == I2C_SLAVE_FORCE) {
        g_i2c_default_addr = (uint8_t)(uintptr_t)arg;
        return 0;
    }

    if (request == I2C_TENBIT) {
        return 0;
    }

    if (request != I2C_RDWR || arg == NULL) {
        return 0;
    }

    struct i2c_rdwr_ioctl_data *rdwr = (struct i2c_rdwr_ioctl_data *)arg;
    if (!rdwr->msgs || rdwr->nmsgs == 0) {
        return -1;
    }

    if (rdwr->nmsgs == 1) {
        struct i2c_msg *m = &rdwr->msgs[0];
        if (m->flags & I2C_M_RD) {
            return hw_i2c_read(0, (uint8_t)m->addr, m->buf, m->len);
        }
        return hw_i2c_write(0, (uint8_t)m->addr, m->buf, m->len);
    }

    if (rdwr->nmsgs == 2 &&
        !(rdwr->msgs[0].flags & I2C_M_RD) &&
        (rdwr->msgs[1].flags & I2C_M_RD) &&
        rdwr->msgs[0].addr == rdwr->msgs[1].addr) {
        return hw_i2c_write_read(0, (uint8_t)rdwr->msgs[0].addr,
                                 rdwr->msgs[0].buf, rdwr->msgs[0].len,
                                 rdwr->msgs[1].buf, rdwr->msgs[1].len);
    }

    for (uint32_t i = 0; i < rdwr->nmsgs; i++) {
        struct i2c_msg *m = &rdwr->msgs[i];
        int rc = (m->flags & I2C_M_RD)
                     ? hw_i2c_read(0, (uint8_t)m->addr, m->buf, m->len)
                     : hw_i2c_write(0, (uint8_t)m->addr, m->buf, m->len);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}

int usleep(useconds_t usec) {
    g_ticks += RTEMS_MICROSECONDS_TO_TICKS((uint32_t)usec);
    return 0;
}

rtems_status_code rtems_task_wake_after(rtems_interval ticks) {
    g_ticks += ticks;
    return RTEMS_SUCCESSFUL;
}

rtems_interval rtems_clock_get_ticks_per_second(void) {
    return 1000;
}

int printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    for (int i = 0; i < n && i < (int)sizeof(buf); i++) {
        hw_uart2_putc(buf[i]);
    }
    return n;
}

__attribute__((weak)) int main(void) { return 0; }
