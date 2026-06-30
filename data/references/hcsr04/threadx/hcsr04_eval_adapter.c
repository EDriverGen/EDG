/* hcsr04_eval_adapter.c — Evaluation adapter for threadx */
#include "drivergen_eval_adapter.h"
#include "hcsr04_ref.h"
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

static void _tx_gpio_set_output_pp(int port, int pin) {
    GPIO_TypeDef *p = (GPIO_TypeDef *)(uintptr_t)(0x40010800 + port * 0x400);
    GPIO_InitTypeDef gi = {0};
    gi.Pin = (uint16_t)(1u << pin); gi.Mode = GPIO_MODE_OUTPUT_PP; gi.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(p, &gi);
}
static void _tx_gpio_set_input_pp(int port, int pin) {
    GPIO_TypeDef *p = (GPIO_TypeDef *)(uintptr_t)(0x40010800 + port * 0x400);
    GPIO_InitTypeDef gi = {0};
    gi.Pin = (uint16_t)(1u << pin); gi.Mode = GPIO_MODE_INPUT; gi.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(p, &gi);
}
static void _tx_gpio_write_pp(int port, int pin, int val) {
    GPIO_TypeDef *p = (GPIO_TypeDef *)(uintptr_t)(0x40010800 + port * 0x400);
    HAL_GPIO_WritePin(p, (uint16_t)(1u << pin), val ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
static int _tx_gpio_read_pp(int port, int pin) {
    GPIO_TypeDef *p = (GPIO_TypeDef *)(uintptr_t)(0x40010800 + port * 0x400);
    return HAL_GPIO_ReadPin(p, (uint16_t)(1u << pin)) == GPIO_PIN_SET ? 1 : 0;
}
static void _tx_delay_us_pp(uint32_t us) { (void)us; }
static uint32_t _tx_get_us_tick_pp(void) { static uint32_t t = 0; return t += 1; }
static const struct hcsr04_platform_ops _gpio_ops = {
    .gpio_set_output = _tx_gpio_set_output_pp,
    .gpio_set_input = _tx_gpio_set_input_pp,
    .gpio_write = _tx_gpio_write_pp,
    .gpio_read = _tx_gpio_read_pp,
    .delay_us = _tx_delay_us_pp,
    .get_us_tick = _tx_get_us_tick_pp,
};

static struct hcsr04_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "hcsr04",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "distance",
    .primary_unit       = "mm",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

/* Parse "PA0".."PH15" into port*16+pin; returns -1 on failure. */
static int hcsr04_eval_parse_pin_n(const char *s, int n) {
    if (s == NULL || n < 3 || s[0] != 'P') return -1;
    char port_c = s[1];
    if (port_c < 'A' || port_c > 'H') return -1;
    int port = (int)(port_c - 'A');
    int pin = 0;
    for (int i = 2; i < n; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        pin = pin * 10 + (s[i] - '0');
        if (pin > 15) return -1;
    }
    return port * 16 + pin;
}

static int hcsr04_eval_parse_pin(const char *s) {
    if (s == NULL) return -1;
    int n = 0; while (s[n] != '\0') n++;
    return hcsr04_eval_parse_pin_n(s, n);
}

/* Parse "PB5:PB6" → trig=21, echo=22. Returns 0 on success, -1 on fail. */
static int hcsr04_eval_parse_trig_echo(const char *s,
                                       int *out_trig, int *out_echo) {
    if (s == NULL) return -1;
    int colon = -1;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == ':') { colon = i; break; }
    }
    if (colon < 0) {
        /* Single pin: echo = trig + 1. */
        int trig = hcsr04_eval_parse_pin(s);
        if (trig < 0) return -1;
        *out_trig = trig;
        *out_echo = trig + 1;
        return 0;
    }
    int trig = hcsr04_eval_parse_pin_n(s, colon);
    int echo = hcsr04_eval_parse_pin(s + colon + 1);
    if (trig < 0 || echo < 0) return -1;
    *out_trig = trig;
    *out_echo = echo;
    return 0;
}

int drivergen_eval_init(const char *bus_name) {
    int port, pin, port2, pin2;
    const char *sep = bus_name;
    while (*sep && *sep != ':') sep++;
    char buf1[8] = {0};
    int l = (int)(sep - bus_name);
    if (l > 7) return DRIVERGEN_EVAL_ERR_INVALID;
    for (int i = 0; i < l; i++) buf1[i] = bus_name[i];
    if (_gpio_parse_pin(buf1, &port, &pin) != 0) return DRIVERGEN_EVAL_ERR_INVALID;
    if (*sep == ':') {
        if (_gpio_parse_pin(sep + 1, &port2, &pin2) != 0) return DRIVERGEN_EVAL_ERR_INVALID;
    } else {
        port2 = port; pin2 = pin + 1;
    }
    int err = hcsr04_init(&g_eval_dev, &_gpio_ops, port, pin, port2, pin2);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int32_t mm = 0;
    if (hcsr04_read_distance_mm(&g_eval_dev, &mm) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = mm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
