/* dht22_eval_adapter.c — Evaluation adapter for tobudos */
#include "drivergen_eval_adapter.h"
#include "dht22_ref.h"
#include "tobudos.h"

#define DHT22_EVAL_CHANNEL_COUNT 2


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

static struct dht22_device g_eval_dev;

static int32_t g_humidity_x10;   /* percent RH * 10 */
static int32_t g_temp_x10;       /* degC * 10 (signed) */
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[DHT22_EVAL_CHANNEL_COUNT] = {
    {"humidity", "pctRH_x10", 0},
    {"temp",     "mC_x10",    0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "dht22",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = DHT22_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "humidity",
    .primary_unit       = "pctRH_x10",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

/* Parse "PA0".."PH15" into RT-Thread GET_PIN(port,pin) = port*16+pin.
 * Returns -1 on parse failure, which the caller surfaces as an init
 * error so the harness fails fast rather than silently using pin 0. */
static int dht22_eval_parse_pin(const char *s) {
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

static int dht22_eval_refresh_cache(void) {
    int16_t  temp_x10 = 0;
    uint16_t hum_x10  = 0;
    if (dht22_read(&g_eval_dev, &temp_x10, &hum_x10) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_humidity_x10  = (int32_t)hum_x10;
    g_temp_x10      = (int32_t)temp_x10;
    g_sample_valid  = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    int port, pin;
    if (_gpio_parse_pin(bus_name, &port, &pin) != 0) return DRIVERGEN_EVAL_ERR_INVALID;
    GPIO_TypeDef *port_ptr = (GPIO_TypeDef *)(uintptr_t)(0x40010800 + port * 0x400);
    int err = dht22_init(&g_eval_dev, port_ptr, (uint16_t)(1u << pin));
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= DHT22_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = dht22_eval_refresh_cache();
        if (err != DRIVERGEN_EVAL_OK) {
            return err;
        }
    }
    *out = (channel_id == 0) ? g_humidity_x10 : g_temp_x10;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
