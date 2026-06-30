/*
 * Apache Mynewt unified stub header for DriverGen evaluation.
 * Covers the OS time subset and HAL I2C/SPI/GPIO types used by drivers.
 */
#ifndef DRIVERGEN_APACHE_MYNEWT_STUB_H
#define DRIVERGEN_APACHE_MYNEWT_STUB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MYNEWT_VAL(x) MYNEWT_VAL_ ## x
#define MYNEWT_VAL_OS_TICKS_PER_SEC 1000
#define OS_TICKS_PER_SEC 1000

typedef int32_t os_stime_t;
typedef uint32_t os_time_t;

#define OS_TIMEOUT_NEVER 0xFFFFFFFFU

os_time_t os_time_get(void);
void os_time_delay(os_time_t osticks);
int os_time_ms_to_ticks(uint32_t ms, os_time_t *out_ticks);
static inline os_time_t os_time_ms_to_ticks32(uint32_t ms) {
    return ms;
}

#define HAL_I2C_ERR_UNKNOWN   1
#define HAL_I2C_ERR_INVAL     2
#define HAL_I2C_ERR_TIMEOUT   3
#define HAL_I2C_ERR_ADDR_NACK 4
#define HAL_I2C_ERR_DATA_NACK 5

struct hal_i2c_hw_settings {
    int pin_scl;
    int pin_sda;
};

struct hal_i2c_settings {
    uint32_t frequency;
};

struct hal_i2c_master_data {
    uint8_t address;
    uint16_t len;
    uint8_t *buffer;
};

int hal_i2c_init(uint8_t i2c_num, void *cfg);
int hal_i2c_init_hw(uint8_t i2c_num, const struct hal_i2c_hw_settings *cfg);
int hal_i2c_enable(uint8_t i2c_num);
int hal_i2c_disable(uint8_t i2c_num);
int hal_i2c_config(uint8_t i2c_num, const struct hal_i2c_settings *cfg);
int hal_i2c_master_write(uint8_t i2c_num, struct hal_i2c_master_data *pdata,
                         uint32_t timeout, uint8_t last_op);
int hal_i2c_master_read(uint8_t i2c_num, struct hal_i2c_master_data *pdata,
                        uint32_t timeout, uint8_t last_op);
int hal_i2c_master_probe(uint8_t i2c_num, uint8_t address, uint32_t timeout);

#define HAL_SPI_TYPE_MASTER 0
#define HAL_SPI_TYPE_SLAVE  1
#define HAL_SPI_MODE0 0
#define HAL_SPI_MODE1 1
#define HAL_SPI_MODE2 2
#define HAL_SPI_MODE3 3
#define HAL_SPI_MSB_FIRST 0
#define HAL_SPI_LSB_FIRST 1
#define HAL_SPI_WORD_SIZE_8BIT 0

typedef void (*hal_spi_txrx_cb)(void *arg, int len);
struct hal_spi_settings {
    uint8_t data_mode;
    uint8_t data_order;
    uint8_t word_size;
    uint32_t baudrate;
};

int hal_spi_init(int spi_num, void *cfg, uint8_t spi_type);
int hal_spi_config(int spi_num, struct hal_spi_settings *psettings);
int hal_spi_enable(int spi_num);
int hal_spi_disable(int spi_num);
uint16_t hal_spi_tx_val(int spi_num, uint16_t val);
int hal_spi_txrx(int spi_num, void *txbuf, void *rxbuf, int cnt);
int hal_spi_txrx_noblock(int spi_num, void *txbuf, void *rxbuf, int cnt);

struct hal_uart_settings {
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t flow_ctl;
};

int hal_uart_init(int uart_num, void *cfg);
int hal_uart_config(int uart_num, const struct hal_uart_settings *settings);
int hal_uart_close(int uart_num);
int hal_uart_blocking_tx(int uart_num, uint8_t byte);
int hal_uart_blocking_rx(int uart_num, uint8_t *byte);

void os_cputime_delay_usecs(uint32_t usecs);

typedef int hal_gpio_pin_t;
typedef void (*hal_gpio_irq_handler_t)(void *arg);

typedef enum hal_gpio_pull {
    HAL_GPIO_PULL_NONE = 0,
    HAL_GPIO_PULL_UP   = 1,
    HAL_GPIO_PULL_DOWN = 2,
} hal_gpio_pull_t;

int hal_gpio_init_out(int pin, int val);
int hal_gpio_init_in(int pin, hal_gpio_pull_t pull);
void hal_gpio_write(int pin, int val);
int hal_gpio_read(int pin);

int printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
