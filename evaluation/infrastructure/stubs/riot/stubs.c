/* RIOT OS stub implementations for DriverGen syntax/link checking. */
#include "riot.h"
#include <string.h>

/* ── I2C ──────────────────────────────────────────────── */
void i2c_init(i2c_t dev) { (void)dev; }
void i2c_acquire(i2c_t dev) { (void)dev; }
void i2c_release(i2c_t dev) { (void)dev; }

int i2c_read_reg(i2c_t dev, uint16_t addr, uint16_t reg,
                 void *data, uint8_t flags) {
    (void)dev; (void)addr; (void)reg; (void)flags;
    if (data) *(uint8_t *)data = 0x19;
    return 0;
}

int i2c_read_regs(i2c_t dev, uint16_t addr, uint16_t reg,
                  void *data, size_t len, uint8_t flags) {
    (void)dev; (void)addr; (void)reg; (void)flags;
    if (data && len > 0) memset(data, 0x19, len);
    return 0;
}

int i2c_read_byte(i2c_t dev, uint16_t addr,
                  void *data, uint8_t flags) {
    (void)dev; (void)addr; (void)flags;
    if (data) *(uint8_t *)data = 0x19;
    return 0;
}

int i2c_read_bytes(i2c_t dev, uint16_t addr,
                   void *data, size_t len, uint8_t flags) {
    (void)dev; (void)addr; (void)flags;
    if (data && len > 0) memset(data, 0x19, len);
    return 0;
}

int i2c_write_byte(i2c_t dev, uint16_t addr,
                   uint8_t data, uint8_t flags) {
    (void)dev; (void)addr; (void)data; (void)flags;
    return 0;
}

int i2c_write_bytes(i2c_t dev, uint16_t addr,
                    const void *data, size_t len, uint8_t flags) {
    (void)dev; (void)addr; (void)data; (void)len; (void)flags;
    return 0;
}

int i2c_write_reg(i2c_t dev, uint16_t addr, uint16_t reg,
                  uint8_t data, uint8_t flags) {
    (void)dev; (void)addr; (void)reg; (void)data; (void)flags;
    return 0;
}

int i2c_write_regs(i2c_t dev, uint16_t addr, uint16_t reg,
                   const void *data, size_t len, uint8_t flags) {
    (void)dev; (void)addr; (void)reg; (void)data; (void)len; (void)flags;
    return 0;
}

/* ── GPIO ─────────────────────────────────────────────── */
int gpio_init(gpio_t pin, gpio_mode_t mode) { (void)pin; (void)mode; return 0; }
int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank,
                  gpio_cb_t cb, void *arg) {
    (void)pin; (void)mode; (void)flank; (void)cb; (void)arg;
    return 0;
}
bool gpio_read(gpio_t pin) { (void)pin; return false; }
void gpio_set(gpio_t pin) { (void)pin; }
void gpio_clear(gpio_t pin) { (void)pin; }
void gpio_toggle(gpio_t pin) { (void)pin; }
void gpio_write(gpio_t pin, bool value) { (void)pin; (void)value; }

/* ── ztimer ───────────────────────────────────────────── */
static ztimer_clock_t _ztimer_usec_inst;
static ztimer_clock_t _ztimer_msec_inst;
static ztimer_clock_t _ztimer_sec_inst;
ztimer_clock_t *ZTIMER_USEC = &_ztimer_usec_inst;
ztimer_clock_t *ZTIMER_MSEC = &_ztimer_msec_inst;
ztimer_clock_t *ZTIMER_SEC  = &_ztimer_sec_inst;

void ztimer_sleep(ztimer_clock_t *clock, uint32_t duration) { (void)clock; (void)duration; }
void ztimer_spin(ztimer_clock_t *clock, uint32_t duration) { (void)clock; (void)duration; }
uint32_t ztimer_now(ztimer_clock_t *clock) { (void)clock; return 0; }
void ztimer_set(ztimer_clock_t *clock, ztimer_t *timer, uint32_t offset) {
    (void)clock; (void)timer; (void)offset;
}
void ztimer_periodic_wakeup(ztimer_clock_t *clock, uint32_t *last_wakeup, uint32_t period) {
    (void)clock; (void)last_wakeup; (void)period;
}

/* ── xtimer (compatibility) ──────────────────────────────────── */
void xtimer_usleep(uint32_t microseconds) { (void)microseconds; }
void xtimer_sleep(uint32_t seconds) { (void)seconds; }
void xtimer_msleep(uint32_t milliseconds) { (void)milliseconds; }
uint32_t xtimer_now_usec(void) { return 0; }

/* ── Mutex ────────────────────────────────────────────── */
void mutex_init(mutex_t *mutex) { (void)mutex; }
void mutex_lock(mutex_t *mutex) { (void)mutex; }
int mutex_trylock(mutex_t *mutex) { (void)mutex; return 1; }
void mutex_unlock(mutex_t *mutex) { (void)mutex; }

/* ── Thread ───────────────────────────────────────────── */
kernel_pid_t thread_create(char *stack, int stacksize,
                           uint8_t priority, int flags,
                           thread_task_func_t task_func,
                           void *arg, const char *name) {
    (void)stack; (void)stacksize; (void)priority; (void)flags;
    (void)task_func; (void)arg; (void)name;
    return 1;
}
void thread_yield(void) {}
kernel_pid_t thread_getpid(void) { return 1; }

/* ── Entry point ──────────────────────────────────────── */
__attribute__((weak)) int main(void) { return 0; }
