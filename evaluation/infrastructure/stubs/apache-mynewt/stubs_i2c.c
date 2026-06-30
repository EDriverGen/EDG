#include "apache_mynewt.h"
#include "hw_i2c.h"
#include "hw_uart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint8_t g_pending_addr;
static uint8_t g_pending_buf[8];
static uint16_t g_pending_len;
static uint8_t g_have_pending;
static os_time_t g_time;

os_time_t os_time_get(void) { return g_time; }
void os_time_delay(os_time_t osticks) { g_time += osticks; }
int os_time_ms_to_ticks(uint32_t ms, os_time_t *out_ticks) {
    if (!out_ticks) {
        return -1;
    }
    *out_ticks = ms;
    return 0;
}

int hal_i2c_init(uint8_t i2c_num, void *cfg) { (void)i2c_num; (void)cfg; hw_i2c1_init(); return 0; }
int hal_i2c_init_hw(uint8_t i2c_num, const struct hal_i2c_hw_settings *cfg) { (void)i2c_num; (void)cfg; hw_i2c1_init(); return 0; }
int hal_i2c_enable(uint8_t i2c_num) { (void)i2c_num; return 0; }
int hal_i2c_disable(uint8_t i2c_num) { (void)i2c_num; return 0; }
int hal_i2c_config(uint8_t i2c_num, const struct hal_i2c_settings *cfg) { (void)i2c_num; (void)cfg; return 0; }

int hal_i2c_master_write(uint8_t i2c_num, struct hal_i2c_master_data *pdata,
                         uint32_t timeout, uint8_t last_op) {
    (void)i2c_num; (void)timeout;
    if (!pdata || (!pdata->buffer && pdata->len > 0)) {
        return HAL_I2C_ERR_INVAL;
    }
    if (!last_op) {
        g_pending_addr = pdata->address;
        g_pending_len = pdata->len > sizeof(g_pending_buf) ? sizeof(g_pending_buf) : pdata->len;
        memcpy(g_pending_buf, pdata->buffer, g_pending_len);
        g_have_pending = 1;
        return 0;
    }
    g_have_pending = 0;
    return hw_i2c_write(0, pdata->address, pdata->buffer, pdata->len) == 0 ? 0 : HAL_I2C_ERR_UNKNOWN;
}

int hal_i2c_master_read(uint8_t i2c_num, struct hal_i2c_master_data *pdata,
                        uint32_t timeout, uint8_t last_op) {
    (void)i2c_num; (void)timeout; (void)last_op;
    if (!pdata || (!pdata->buffer && pdata->len > 0)) {
        return HAL_I2C_ERR_INVAL;
    }
    if (g_have_pending && g_pending_addr == pdata->address) {
        g_have_pending = 0;
        return hw_i2c_write_read(0, pdata->address, g_pending_buf, g_pending_len,
                                 pdata->buffer, pdata->len) == 0 ? 0 : HAL_I2C_ERR_UNKNOWN;
    }
    return hw_i2c_read(0, pdata->address, pdata->buffer, pdata->len) == 0 ? 0 : HAL_I2C_ERR_UNKNOWN;
}

int hal_i2c_master_probe(uint8_t i2c_num, uint8_t address, uint32_t timeout) {
    (void)i2c_num; (void)address; (void)timeout; return 0;
}

int hal_gpio_init_out(int pin, int val) { (void)pin; (void)val; return 0; }
int hal_gpio_init_in(int pin, hal_gpio_pull_t pull) { (void)pin; (void)pull; return 0; }
void hal_gpio_write(int pin, int val) { (void)pin; (void)val; }
int hal_gpio_read(int pin) { (void)pin; return 0; }

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
