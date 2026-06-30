/* ds18b20_eval_adapter.c — Evaluation adapter for threadx */
#include "drivergen_eval_adapter.h"
#include "ds18b20_ref.h"
#include "threadx.h"


static int _gpio_parse_pin(const char *s, int *port_out, int *pin_out) {
    if (!s || s[0] != 'P') return -1;
    char pc = s[1];
    if (pc < 'A' || pc > 'H') return -1;
    *port_out = (int)(pc - 'A');
    int pin = 0;
    for (int i = 2; s[i]; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        pin = pin * 10 + (s[i] - '0');
        if (pin > 15) return -1;
    }
    *pin_out = pin;
    return 0;
}
static GPIO_TypeDef *_gpio_port;
static uint16_t _gpio_pin_mask;

static void _tx_gpio_set_output(void *ctx) {
    (void)ctx;
    GPIO_InitTypeDef gi = {0};
    gi.Pin = _gpio_pin_mask; gi.Mode = GPIO_MODE_OUTPUT_PP; gi.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(_gpio_port, &gi);
}
static void _tx_gpio_set_input(void *ctx) {
    (void)ctx;
    GPIO_InitTypeDef gi = {0};
    gi.Pin = _gpio_pin_mask; gi.Mode = GPIO_MODE_INPUT; gi.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(_gpio_port, &gi);
}
static void _tx_gpio_write(void *ctx, int val) {
    (void)ctx;
    HAL_GPIO_WritePin(_gpio_port, _gpio_pin_mask, val ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
static int _tx_gpio_read(void *ctx) {
    (void)ctx;
    return HAL_GPIO_ReadPin(_gpio_port, _gpio_pin_mask) == GPIO_PIN_SET ? 1 : 0;
}
static void _tx_gpio_delay_us(uint32_t us) { (void)us; }
static void _tx_gpio_delay_ms(uint32_t ms) { HAL_Delay(ms); }
static const struct ds18b20_gpio_ops _gpio_ops = {
    .set_output = _tx_gpio_set_output,
    .set_input = _tx_gpio_set_input,
    .write = _tx_gpio_write,
    .read = _tx_gpio_read,
    .delay_us = _tx_gpio_delay_us,
    .delay_ms = _tx_gpio_delay_ms,
};

static struct ds18b20_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "ds18b20",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "temp",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

/* Parse "PA0".."PH15" into RT-Thread GET_PIN(port,pin) = port*16+pin.
 * Returns -1 on parse failure. Matches dht22 / hcsr04 convention. */
static int ds18b20_eval_parse_pin(const char *s) {
    if (s == NULL || s[0] != 'P') return -1;
    char port_c = s[1];
    if (port_c < 'A' || port_c > 'H') return -1;
    int port = (int)(port_c - 'A');
    int pin = 0;
    for (int i = 2; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        pin = pin * 10 + (s[i] - '0');
        if (pin > 15) return -1;
    }
    return port * 16 + pin;
}

int drivergen_eval_init(const char *bus_name) {
    int port, pin;
    if (_gpio_parse_pin(bus_name, &port, &pin) != 0) return DRIVERGEN_EVAL_ERR_INVALID;
    _gpio_port = (GPIO_TypeDef *)(uintptr_t)(0x40010800 + port * 0x400);
    _gpio_pin_mask = (uint16_t)(1u << pin);
    int err = ds18b20_init(&g_eval_dev, &_gpio_ops, NULL);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int32_t cc = 0;  /* centicelsius */
    if (ds18b20_read_temp(&g_eval_dev, &cc) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = cc * 10;  /* cC → mC */
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
