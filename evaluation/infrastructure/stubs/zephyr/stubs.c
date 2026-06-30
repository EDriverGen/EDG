/* Zephyr stub implementations */
#include "zephyr.h"
#include <stdlib.h>

bool device_is_ready(const struct device *dev) { (void)dev; return true; }

/* I2C stubs */
int i2c_write(const struct device *d, const uint8_t *b, uint32_t n, uint16_t a) {
    (void)d; (void)b; (void)n; (void)a; return 0;
}
int i2c_read(const struct device *d, uint8_t *b, uint32_t n, uint16_t a) {
    (void)d; (void)a; if (b && n) memset(b, 0x19, n); return 0;
}
int i2c_write_read(const struct device *d, uint16_t a,
                   const void *wb, size_t nw, void *rb, size_t nr) {
    (void)d; (void)a; (void)wb; (void)nw;
    if (rb && nr) memset(rb, 0x19, nr); return 0;
}
int i2c_transfer(const struct device *d, struct i2c_msg *m, uint8_t n, uint16_t a) {
    (void)d; (void)m; (void)n; (void)a; return 0;
}
int i2c_write_dt(const struct i2c_dt_spec *s, const uint8_t *b, uint32_t n) {
    (void)s; (void)b; (void)n; return 0;
}
int i2c_read_dt(const struct i2c_dt_spec *s, uint8_t *b, uint32_t n) {
    (void)s; if (b && n) memset(b, 0x19, n); return 0;
}
int i2c_write_read_dt(const struct i2c_dt_spec *s,
                      const void *wb, size_t nw, void *rb, size_t nr) {
    (void)s; (void)wb; (void)nw;
    if (rb && nr) memset(rb, 0x19, nr); return 0;
}
int i2c_burst_read(const struct device *d, uint16_t da, uint8_t sa, uint8_t *b, uint32_t n) {
    (void)d; (void)da; (void)sa; if (b && n) memset(b, 0x19, n); return 0;
}
int i2c_burst_write(const struct device *d, uint16_t da, uint8_t sa, const uint8_t *b, uint32_t n) {
    (void)d; (void)da; (void)sa; (void)b; (void)n; return 0;
}
int i2c_reg_read_byte(const struct device *d, uint16_t da, uint8_t ra, uint8_t *v) {
    (void)d; (void)da; (void)ra; if (v) *v = 0x19; return 0;
}
int i2c_reg_write_byte(const struct device *d, uint16_t da, uint8_t ra, uint8_t v) {
    (void)d; (void)da; (void)ra; (void)v; return 0;
}
int i2c_reg_read_byte_dt(const struct i2c_dt_spec *s, uint8_t ra, uint8_t *v) {
    (void)s; (void)ra; if (v) *v = 0x19; return 0;
}
int i2c_reg_write_byte_dt(const struct i2c_dt_spec *s, uint8_t ra, uint8_t v) {
    (void)s; (void)ra; (void)v; return 0;
}
int i2c_reg_update_byte_dt(const struct i2c_dt_spec *s, uint8_t ra, uint8_t mask, uint8_t value) {
    (void)s; (void)ra; (void)mask; (void)value; return 0;
}
int i2c_burst_read_dt(const struct i2c_dt_spec *s, uint8_t sa, uint8_t *b, uint32_t n) {
    (void)s; (void)sa; if (b && n) memset(b, 0x19, n); return 0;
}
int i2c_burst_write_dt(const struct i2c_dt_spec *s, uint8_t sa, const uint8_t *b, uint32_t n) {
    (void)s; (void)sa; (void)b; (void)n; return 0;
}
int i2c_transfer_dt(const struct i2c_dt_spec *s, struct i2c_msg *m, uint8_t n) {
    (void)s; (void)m; (void)n; return 0;
}

/* GPIO stubs */
int gpio_pin_configure(const struct device *p, gpio_pin_t pin, gpio_flags_t f) {
    (void)p; (void)pin; (void)f; return 0;
}
int gpio_pin_configure_dt(const struct gpio_dt_spec *s, gpio_flags_t f) {
    (void)s; (void)f; return 0;
}
int gpio_pin_set(const struct device *p, gpio_pin_t pin, int v) {
    (void)p; (void)pin; (void)v; return 0;
}
int gpio_pin_set_dt(const struct gpio_dt_spec *s, int v) { (void)s; (void)v; return 0; }
int gpio_pin_get(const struct device *p, gpio_pin_t pin) { (void)p; (void)pin; return 0; }
int gpio_pin_get_dt(const struct gpio_dt_spec *s) { (void)s; return 0; }
int gpio_pin_toggle_dt(const struct gpio_dt_spec *s) { (void)s; return 0; }

/* Kernel timing stubs */
void k_msleep(int32_t ms) { (void)ms; }
void k_usleep(int32_t us) { (void)us; }
void k_busy_wait(uint32_t usec) { (void)usec; }
int64_t k_uptime_get(void) { return 0; }
uint32_t k_uptime_get_32(void) { return 0; }
k_ticks_t k_uptime_ticks(void) { return 0; }
int32_t k_sleep(k_timeout_t timeout) { (void)timeout; return 0; }

__attribute__((weak)) int main(void) { return 0; }

/* device_get_binding(name): device lookup. Returns non-NULL sentinel. */
const struct device *device_get_binding(const char *name) {
    static const struct device _dummy = { .name = "stub", .config = 0, .api = 0, .data = 0 };
    (void)name;
    return &_dummy;
}
