#include "apache_mynewt.h"
#include "hw_spi.h"
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

int hal_spi_init(int spi_num, void *cfg, uint8_t spi_type)
{
    (void)spi_num;
    (void)cfg;
    (void)spi_type;
    hw_spi1_init();
    return 0;
}

int hal_spi_config(int spi_num, struct hal_spi_settings *psettings)
{
    (void)spi_num;
    (void)psettings;
    return 0;
}

int hal_spi_enable(int spi_num)
{
    (void)spi_num;
    return 0;
}

int hal_spi_disable(int spi_num)
{
    (void)spi_num;
    return 0;
}

uint16_t hal_spi_tx_val(int spi_num, uint16_t val)
{
    (void)spi_num;
    hw_spi1_cs_lo();
    uint8_t rx = hw_spi1_xfer_byte((uint8_t)val);
    hw_spi1_cs_hi();
    return rx;
}

int hal_spi_txrx(int spi_num, void *txbuf, void *rxbuf, int cnt)
{
    uint8_t *tx = (uint8_t *)txbuf;
    uint8_t *rx = (uint8_t *)rxbuf;
    (void)spi_num;
    if (cnt < 0) {
        return -1;
    }
    hw_spi1_cs_lo();
    for (int i = 0; i < cnt; i++) {
        uint8_t b = tx != 0 ? tx[i] : 0x00;
        uint8_t r = hw_spi1_xfer_byte(b);
        if (rx != 0) {
            rx[i] = r;
        }
    }
    hw_spi1_cs_hi();
    return 0;
}

int hal_spi_txrx_noblock(int spi_num, void *txbuf, void *rxbuf, int cnt)
{
    return hal_spi_txrx(spi_num, txbuf, rxbuf, cnt);
}

int hal_gpio_init_out(int pin, int val) { (void)pin; (void)val; return 0; }
int hal_gpio_init_in(int pin, hal_gpio_pull_t pull) { (void)pin; (void)pull; return 0; }
void hal_gpio_write(int pin, int val) { (void)pin; (void)val; }
int hal_gpio_read(int pin) { (void)pin; return 0; }

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
