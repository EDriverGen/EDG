#include "rtems.h"
#include "dev/spi/spi.h"
#include "hw_spi.h"
#include "hw_uart.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

static int g_next_fd = 3;
static rtems_interval g_ticks;
static uint8_t g_spi_mode = SPI_MODE_0;
static uint8_t g_spi_bits = 8;
static uint32_t g_spi_speed = 1000000U;

int open(const char *path, int oflag, ...)
{
    (void)path;
    (void)oflag;
    hw_spi1_init();
    return g_next_fd++;
}

int close(int fd)
{
    (void)fd;
    return 0;
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
    (void)fd;
    return hw_spi1_transfer(0, (uint8_t *)buf, nbyte) == 0 ? (ssize_t)nbyte : -1;
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
    (void)fd;
    return hw_spi1_transfer((const uint8_t *)buf, 0, nbyte) == 0 ? (ssize_t)nbyte : -1;
}

static int rtems_spi_message(struct spi_ioc_transfer *msgs, unsigned int nmsgs)
{
    if (msgs == 0 || nmsgs == 0) {
        return -1;
    }

    hw_spi1_cs_lo();
    for (unsigned int m = 0; m < nmsgs; m++) {
        const uint8_t *tx = (const uint8_t *)(uintptr_t)msgs[m].tx_buf;
        uint8_t *rx = (uint8_t *)(uintptr_t)msgs[m].rx_buf;
        for (uint32_t i = 0; i < msgs[m].len; i++) {
            uint8_t b = tx != 0 ? tx[i] : 0x00;
            uint8_t r = hw_spi1_xfer_byte(b);
            if (rx != 0) {
                rx[i] = r;
            }
        }
        if (msgs[m].cs_change && m + 1U < nmsgs) {
            hw_spi1_cs_hi();
            hw_spi1_cs_lo();
        }
    }
    hw_spi1_cs_hi();
    return 0;
}

int ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    va_list ap;
    va_start(ap, request);
    void *arg = va_arg(ap, void *);
    va_end(ap);

    if (request == SPI_BUS_OBTAIN || request == SPI_BUS_RELEASE) {
        return 0;
    }
    if (request == SPI_IOC_WR_MODE && arg != 0) {
        g_spi_mode = *(uint8_t *)arg;
        return 0;
    }
    if (request == SPI_IOC_RD_MODE && arg != 0) {
        *(uint8_t *)arg = g_spi_mode;
        return 0;
    }
    if (request == SPI_IOC_WR_BITS_PER_WORD && arg != 0) {
        g_spi_bits = *(uint8_t *)arg;
        return 0;
    }
    if (request == SPI_IOC_RD_BITS_PER_WORD && arg != 0) {
        *(uint8_t *)arg = g_spi_bits;
        return 0;
    }
    if (request == SPI_IOC_WR_MAX_SPEED_HZ && arg != 0) {
        g_spi_speed = *(uint32_t *)arg;
        return 0;
    }
    if (request == SPI_IOC_RD_MAX_SPEED_HZ && arg != 0) {
        *(uint32_t *)arg = g_spi_speed;
        return 0;
    }
    if ((request & 0xFF00u) == 0x6C00u) {
        unsigned int nmsgs = (unsigned int)(request & 0xFFu);
        return rtems_spi_message((struct spi_ioc_transfer *)arg, nmsgs);
    }
    return 0;
}

rtems_status_code rtems_task_wake_after(rtems_interval ticks)
{
    g_ticks += ticks;
    return RTEMS_SUCCESSFUL;
}

rtems_interval rtems_clock_get_ticks_per_second(void)
{
    return 1000;
}

int printf(const char *fmt, ...)
{
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
