/* bme280_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the BME280 RT-Thread
 * reference driver (I2C multi_channel, 3 channels: temp, pressure,
 * humidity). Channel order mirrors the oracle meta.channels[].
 *
 * Reference driver API:
 *   int bme280_init(dev, struct rt_i2c_bus_device *bus, uint16_t addr);
 *   int bme280_probe(dev);
 *   int bme280_read_calibration(dev);
 *   int bme280_read(dev, int32_t *temp_mc,
 *                         uint32_t *press_pa,
 *                         uint32_t *hum_mp);
 *
 * The driver returns three values atomically in one call. We cache
 * them on read_channel(0) and serve subsequent channels from cache.
 *
 * Units:
 *   channel 0 (temp)      : mC (matches oracle)
 *   channel 1 (pressure)  : Pa (matches oracle)
 *   channel 2 (humidity)  : driver's `hum_mp` = %RH * 1024 (Q22.10 form
 *                          from Bosch compensation). Oracle declares
 *                          `pctRH_x1024`, so no rescale is applied.
 */
#include "drivergen_eval_adapter.h"
#include "bme280_ref.h"

#define BME280_EVAL_CHANNEL_COUNT 3

static struct bme280_device g_eval_dev;
static int32_t g_cached[BME280_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[BME280_EVAL_CHANNEL_COUNT] = {
    {"temp",     "mC",           0},
    {"pressure", "Pa",           0},
    {"humidity", "pctRH_x1024",  0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "bme280",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = BME280_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "temp",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int bme280_eval_refresh_cache(void) {
    int32_t  temp_mc  = 0;
    uint32_t press_pa = 0;
    uint32_t hum_mp   = 0;
    if (bme280_read(&g_eval_dev, &temp_mc, &press_pa, &hum_mp) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = temp_mc;
    g_cached[1] = (int32_t)press_pa;
    g_cached[2] = (int32_t)hum_mp;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_device_t dev = rt_device_find(bus_name);
    if (dev == RT_NULL) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev;
    if (bme280_init(&g_eval_dev, bus, BME280_ADDR_DEFAULT) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (bme280_probe(&g_eval_dev) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (bme280_read_calibration(&g_eval_dev) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= BME280_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = bme280_eval_refresh_cache();
        if (err != DRIVERGEN_EVAL_OK) {
            return err;
        }
    }
    *out = g_cached[channel_id];
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
