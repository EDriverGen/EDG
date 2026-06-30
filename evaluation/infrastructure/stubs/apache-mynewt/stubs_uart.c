#include "apache_mynewt.h"
#include "hw_uart_bus.h"
#include "hw_uart.h"
#include <stdarg.h>
#include <stdio.h>

static os_time_t g_time;

os_time_t os_time_get(void) { return g_time; }
void os_time_delay(os_time_t osticks) { g_time += osticks; }
int os_time_ms_to_ticks(uint32_t ms, os_time_t *out_ticks)
{
    if (out_ticks == 0) {
        return -1;
    }
    *out_ticks = ms;
    return 0;
}

void os_cputime_delay_usecs(uint32_t usecs)
{
    (void)usecs;
}

int hal_uart_init(int uart_num, void *cfg)
{
    (void)uart_num;
    (void)cfg;
    hw_uart_bus_init();
    return 0;
}

int hal_uart_config(int uart_num, const struct hal_uart_settings *settings)
{
    (void)uart_num;
    (void)settings;
    return 0;
}

int hal_uart_close(int uart_num)
{
    (void)uart_num;
    return 0;
}

int hal_uart_blocking_tx(int uart_num, uint8_t byte)
{
    (void)uart_num;
    hw_uart_bus_write_byte(byte);
    return 0;
}

int hal_uart_blocking_rx(int uart_num, uint8_t *byte)
{
    (void)uart_num;
    if (byte == 0) {
        return -1;
    }
    return hw_uart_bus_read_byte(byte);
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
