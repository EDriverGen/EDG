#include "rtems.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

static int g_next_fd = 3;
static rtems_interval g_ticks;

int open(const char *path, int oflag, ...)
{
    (void)path;
    (void)oflag;
    hw_uart_bus_init();
    return g_next_fd++;
}

int close(int fd)
{
    (void)fd;
    return 0;
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
    const uint8_t *p = (const uint8_t *)buf;
    (void)fd;
    for (size_t i = 0; i < nbyte; i++) {
        hw_uart_bus_write_byte(p[i]);
    }
    return (ssize_t)nbyte;
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
    uint8_t *p = (uint8_t *)buf;
    (void)fd;
    for (size_t i = 0; i < nbyte; i++) {
        if (hw_uart_bus_read_byte(&p[i]) != 0) {
            return (ssize_t)i;
        }
    }
    return (ssize_t)nbyte;
}

int ioctl(int fd, unsigned long request, ...)
{
    (void)fd;
    (void)request;
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
