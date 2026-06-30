/* dht22_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the DHT22/AM2302
 * RT-Thread reference driver (GPIO 1-wire pulse-timed protocol).
 *
 * Provides the minimal eval ABI surface for the "multi_channel"
 * eval_class with 2 channels: humidity, temp (order matches the oracle
 * meta.channels[] list).
 *
 * Reference driver API:
 *   rt_err_t dht22_init(dev, rt_base_t data_pin);
 *   rt_err_t dht22_read(dev, int16_t *temp_x10, uint16_t *humidity_x10);
 *
 * `bus_name` is a pin label string like "PB5"; we parse it into an
 * integer that `rt_pin_read/write` will treat opaquely. The convention
 * matches RT-Thread's `GET_PIN(port,pin) = port_index*16 + pin_index`:
 *     PA0  -> 0        PA15 -> 15
 *     PB0  -> 16       PB5  -> 21    PB15 -> 31
 *     PC0  -> 32       ...
 *
 * Caching model matches the other multi_channel adapters (MPU6050,
 * ADXL345): the first read_channel(0) call performs a full device
 * read, populates all channel values, and subsequent channel reads
 * return cached values from the same physical frame.
 *
 * Runtime note: the evaluation framework's rtthread stubs currently
 * implement `rt_pin_read` as a returns-0 stub (see
 * evaluation/infrastructure/stubs/rtthread/stubs_i2c.c). End-to-end
 * Renode runs against the gpio_pulse_injector require replacing that
 * stub with a memory-mapped read from GPIOB_IDR; filed under
 * EVAL-FUTURE as the DHT22 E17 baseline runtime caveat.
 */
#include "drivergen_eval_adapter.h"
#include "dht22_ref.h"

#define DHT22_EVAL_CHANNEL_COUNT 2

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
    if (dht22_read(&g_eval_dev, &temp_x10, &hum_x10) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_humidity_x10  = (int32_t)hum_x10;
    g_temp_x10      = (int32_t)temp_x10;
    g_sample_valid  = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    int pin_id = dht22_eval_parse_pin(bus_name);
    if (pin_id < 0) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (dht22_init(&g_eval_dev, (rt_base_t)pin_id) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
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
