#ifndef __ZEPHYR_STUB_H__
#define __ZEPHYR_STUB_H__
/*
 * Zephyr RTOS API stubs for cross-compilation testing.
 * Covers: device model, I2C, GPIO, kernel timing.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "zephyr/sys/util.h"

/* ---- Zephyr types ---- */
typedef int32_t k_timeout_t;
typedef uint32_t k_ticks_t;

/* ---- Device model ---- */
struct device {
    const char *name;
    const void *config;
    const void *api;
    void *data;
};

bool device_is_ready(const struct device *dev);
#define DEVICE_DT_GET(node_id)        ((const struct device *)0)
#define DEVICE_DT_GET_OR_NULL(node_id) ((const struct device *)0)

/*
 * device_get_binding(name) — Zephyr device lookup API, used by pre-DT
 * sample that queries a peripheral by string label ("I2C_1" / "i2c@40005400").
 * Upstream marks it "deprecated but not removed"; for stub-compile purposes we
 * return a non-NULL sentinel so the driver's own NULL-check passes without
 * us having to track a real device registry.  The returned pointer must not
 * be dereferenced by the stub consumer — it is opaque.
 */
const struct device *device_get_binding(const char *name);

/* ---- I2C ---- */
struct i2c_dt_spec {
    const struct device *bus;
    uint16_t addr;
};

#define I2C_DT_SPEC_GET(node_id)  { .bus = NULL, .addr = 0 }
#define I2C_SPEED_STANDARD  0x1
#define I2C_SPEED_FAST      0x2
#define I2C_MSG_WRITE       0
#define I2C_MSG_READ        (1 << 0)
#define I2C_MSG_STOP        (1 << 1)
#define I2C_MSG_RESTART     (1 << 2)

struct i2c_msg {
    uint8_t *buf;
    uint32_t len;
    uint8_t flags;
};

int i2c_write(const struct device *dev, const uint8_t *buf, uint32_t num_bytes, uint16_t addr);
int i2c_read(const struct device *dev, uint8_t *buf, uint32_t num_bytes, uint16_t addr);
int i2c_write_read(const struct device *dev, uint16_t addr,
                   const void *write_buf, size_t num_write,
                   void *read_buf, size_t num_read);
int i2c_transfer(const struct device *dev, struct i2c_msg *msgs,
                 uint8_t num_msgs, uint16_t addr);
int i2c_write_dt(const struct i2c_dt_spec *spec, const uint8_t *buf, uint32_t num_bytes);
int i2c_read_dt(const struct i2c_dt_spec *spec, uint8_t *buf, uint32_t num_bytes);
int i2c_write_read_dt(const struct i2c_dt_spec *spec,
                      const void *write_buf, size_t num_write,
                      void *read_buf, size_t num_read);
int i2c_burst_read(const struct device *dev, uint16_t dev_addr,
                   uint8_t start_addr, uint8_t *buf, uint32_t num_bytes);
int i2c_burst_write(const struct device *dev, uint16_t dev_addr,
                    uint8_t start_addr, const uint8_t *buf, uint32_t num_bytes);
int i2c_reg_read_byte(const struct device *dev, uint16_t dev_addr,
                      uint8_t reg_addr, uint8_t *value);
int i2c_reg_write_byte(const struct device *dev, uint16_t dev_addr,
                       uint8_t reg_addr, uint8_t value);
int i2c_reg_read_byte_dt(const struct i2c_dt_spec *spec,
                         uint8_t reg_addr, uint8_t *value);
int i2c_reg_write_byte_dt(const struct i2c_dt_spec *spec,
                          uint8_t reg_addr, uint8_t value);
int i2c_reg_update_byte_dt(const struct i2c_dt_spec *spec,
                           uint8_t reg_addr, uint8_t mask, uint8_t value);
int i2c_burst_read_dt(const struct i2c_dt_spec *spec,
                      uint8_t start_addr, uint8_t *buf, uint32_t num_bytes);
int i2c_burst_write_dt(const struct i2c_dt_spec *spec,
                       uint8_t start_addr, const uint8_t *buf, uint32_t num_bytes);
int i2c_transfer_dt(const struct i2c_dt_spec *spec,
                    struct i2c_msg *msgs, uint8_t num_msgs);

/*
 * Zephyr provides i2c_is_ready_dt() as a device-ready check tailored to
 * the bus pointer carried by i2c_dt_spec.  Upstream defines it as a
 * static inline in include/zephyr/drivers/i2c.h; we mirror that here so
 * drivers that pattern-match modern Zephyr samples compile cleanly
 * without needing an extra stub translation unit.
 */
static inline bool i2c_is_ready_dt(const struct i2c_dt_spec *spec)
{
    return (spec != NULL) && device_is_ready(spec->bus);
}

/* ---- GPIO ---- */
typedef uint32_t gpio_flags_t;
typedef uint32_t gpio_pin_t;

struct gpio_dt_spec {
    const struct device *port;
    gpio_pin_t pin;
    gpio_flags_t dt_flags;
};

#define GPIO_DT_SPEC_GET(node_id, prop, idx) { .port = NULL, .pin = 0, .dt_flags = 0 }
#define GPIO_OUTPUT       (1 << 1)
#define GPIO_OUTPUT_INIT_LOW  (GPIO_OUTPUT | (0 << 5))
#define GPIO_OUTPUT_INIT_HIGH (GPIO_OUTPUT | (1 << 5))
#define GPIO_OUTPUT_LOW       GPIO_OUTPUT_INIT_LOW
#define GPIO_OUTPUT_HIGH      GPIO_OUTPUT_INIT_HIGH
#define GPIO_OUTPUT_INACTIVE  GPIO_OUTPUT_INIT_LOW
#define GPIO_OUTPUT_ACTIVE    GPIO_OUTPUT_INIT_HIGH
#define GPIO_INPUT        (1 << 0)
#define GPIO_PULL_UP      (1 << 4)
#define GPIO_PULL_DOWN    (1 << 5)
#define GPIO_ACTIVE_LOW   (1 << 2)
#define GPIO_ACTIVE_HIGH  0

int gpio_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags);
int gpio_pin_configure_dt(const struct gpio_dt_spec *spec, gpio_flags_t extra_flags);
int gpio_pin_set(const struct device *port, gpio_pin_t pin, int value);
int gpio_pin_set_dt(const struct gpio_dt_spec *spec, int value);
int gpio_pin_get(const struct device *port, gpio_pin_t pin);
int gpio_pin_get_dt(const struct gpio_dt_spec *spec);
int gpio_pin_toggle_dt(const struct gpio_dt_spec *spec);

/* ---- Kernel timing ---- */

/* ---- SPI ---- */
struct spi_cs_control {
    struct gpio_dt_spec gpio;
    uint32_t delay;
};

struct spi_config {
    uint32_t frequency;
    uint16_t operation;
    uint16_t slave;
    struct spi_cs_control *cs;
};

struct spi_dt_spec {
    const struct device *bus;
    struct spi_config config;
};

struct spi_buf {
    void *buf;
    size_t len;
};

struct spi_buf_set {
    const struct spi_buf *buffers;
    size_t count;
};

#define SPI_WORD_SET(n)          (((n) - 1) << 1)
#define SPI_OP_MODE_MASTER       0
#define SPI_OP_MODE_SLAVE        (1 << 0)
#define SPI_MODE_CPOL            (1 << 5)
#define SPI_MODE_CPHA            (1 << 6)
#define SPI_TRANSFER_MSB         0
#define SPI_TRANSFER_LSB         (1 << 7)
#define SPI_CS_ACTIVE_HIGH       (1 << 8)
#define SPI_LOCK_ON              (1 << 9)
#define SPI_FULL_DUPLEX          0
#define SPI_HALF_DUPLEX          (1 << 10)
#define SPI_MODE_GET(mode)       (((mode) >> 5) & 0x3)

#define SPI_DT_SPEC_GET(node_id, prop, operate_, delay_) \
    { .bus = NULL, .config = { .frequency = 0, .operation = (operate_), .slave = 0 } }

int spi_transceive(const struct device *dev, const struct spi_config *config,
                   const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs);
int spi_transceive_dt(const struct spi_dt_spec *spec,
                      const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs);
int spi_read(const struct device *dev, const struct spi_config *config,
             const struct spi_buf_set *rx_bufs);
int spi_read_dt(const struct spi_dt_spec *spec, const struct spi_buf_set *rx_bufs);
int spi_write(const struct device *dev, const struct spi_config *config,
              const struct spi_buf_set *tx_bufs);
int spi_write_dt(const struct spi_dt_spec *spec, const struct spi_buf_set *tx_bufs);

/* ---- UART ---- */
struct uart_config {
    uint32_t baudrate;
    uint8_t  parity;
    uint8_t  stop_bits;
    uint8_t  data_bits;
    uint8_t  flow_ctrl;
};

#define UART_CFG_PARITY_NONE   0
#define UART_CFG_PARITY_ODD    1
#define UART_CFG_PARITY_EVEN   2
#define UART_CFG_STOP_BITS_1   1
#define UART_CFG_STOP_BITS_2   2
#define UART_CFG_DATA_BITS_8   8
#define UART_CFG_FLOW_CTRL_NONE 0

typedef void (*uart_irq_callback_user_data_t)(const struct device *dev, void *user_data);

int uart_configure(const struct device *dev, const struct uart_config *cfg);
int uart_config_get(const struct device *dev, struct uart_config *cfg);
void uart_poll_out(const struct device *dev, unsigned char out_char);
int uart_poll_in(const struct device *dev, unsigned char *p_char);
int uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size);
int uart_fifo_read(const struct device *dev, uint8_t *rx_data, int size);
void uart_irq_rx_enable(const struct device *dev);
void uart_irq_rx_disable(const struct device *dev);
void uart_irq_tx_enable(const struct device *dev);
void uart_irq_tx_disable(const struct device *dev);
int uart_irq_rx_ready(const struct device *dev);
int uart_irq_tx_ready(const struct device *dev);
void uart_irq_callback_user_data_set(const struct device *dev,
                                     uart_irq_callback_user_data_t cb, void *user_data);

/* ---- Kernel timing ---- */
void k_msleep(int32_t ms);
void k_usleep(int32_t us);
void k_busy_wait(uint32_t usec_to_wait);
int64_t k_uptime_get(void);
uint32_t k_uptime_get_32(void);
k_ticks_t k_uptime_ticks(void);

/*
 * k_sleep() returns ticks remaining on wakeup; upstream defines
 * k_timeout_t as a struct, but we keep it a typedef'd int32_t above to
 * keep stubs uniform.  K_MSEC / K_USEC / K_NO_WAIT are passed by value;
 * drivers rarely inspect the struct fields, so a plain integer
 * encoding (ms * 1000 + flag) is sufficient for compile-time checks.
 */
#define K_NO_WAIT            ((k_timeout_t)0)
#define K_FOREVER            ((k_timeout_t)-1)
#define K_MSEC(ms)           ((k_timeout_t)(ms))
#define K_USEC(us)           ((k_timeout_t)(((us) + 999) / 1000))
#define K_SECONDS(s)         ((k_timeout_t)((s) * 1000))
#define K_MINUTES(m)         ((k_timeout_t)((m) * 60 * 1000))

int32_t k_sleep(k_timeout_t timeout);

/* ---- Kernel thread ---- */
#define K_THREAD_STACK_DEFINE(sym, size) char sym[size]
#define K_THREAD_DEFINE(name, stack_size, entry, p1, p2, p3, prio, options, delay) \
    void entry(void *, void *, void *)

/* ---- Logging ---- */
#ifndef LOG_MODULE_REGISTER
#define LOG_MODULE_REGISTER(...)
#endif
#ifndef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)
#endif
#ifndef LOG_ERR
#define LOG_ERR(...) do { } while (0)
#endif
#ifndef LOG_WRN
#define LOG_WRN(...) do { } while (0)
#endif
#ifndef LOG_INF
#define LOG_INF(...) do { } while (0)
#endif
#ifndef LOG_DBG
#define LOG_DBG(...) do { } while (0)
#endif
#ifndef LOG_LEVEL_ERR
#define LOG_LEVEL_ERR 1
#endif
#ifndef LOG_LEVEL_WRN
#define LOG_LEVEL_WRN 2
#endif
#ifndef LOG_LEVEL_INF
#define LOG_LEVEL_INF 3
#endif
#ifndef LOG_LEVEL_DBG
#define LOG_LEVEL_DBG 4
#endif

/* ---- Devicetree ---- */
#define DT_NODELABEL(label)    0
#define DT_ALIAS(alias)        0
#define DT_INST(inst, compat)  0
#define DT_NODE_HAS_STATUS(node_id, status) 1
#define DT_PROP(node_id, prop) 0

/* ---- Sys (functions provided by zephyr/sys/byteorder.h) ---- */

/* ---- Printk ---- */
#define printk printf

/* ---- Error codes ---- */
#define ENODEV  19
#define EIO     5
#define EINVAL  22
#define EBUSY   16

#endif /* __ZEPHYR_STUB_H__ */
