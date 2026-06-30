/* dps310_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the DPS310 RT-Thread
 * reference driver (I2C multi_channel, 2 channels: temp, pressure).
 *
 * Reference driver API:
 *   rt_err_t dps310_init(dev, const char *bus_name, rt_uint8_t addr);
 *   rt_err_t dps310_probe(dev);
 *   rt_err_t dps310_reset(dev);
 *   rt_err_t dps310_read_calibration(dev);
 *   rt_err_t dps310_read_temperature(dev, rt_int32_t *temp_c100);
 *   rt_err_t dps310_read_pressure   (dev, rt_int32_t *pressure_pa100);
 *
 * The driver exposes two separate reads. The adapter refreshes both on
 * read_channel(0) and serves the cached values on subsequent reads.
 *
 * Unit mapping (driver → oracle):
 *   temp_c100       (0.01 degC) → *10 to become mC
 *   pressure_pa100  (0.01 Pa)   → /100 to become Pa
 *
 * Note: `dps310_read_pressure` requires a prior temperature measurement
 * for compensation, so the cache-refresh always calls the temp read first.
 */
#include "drivergen_eval_adapter.h"
#include "dps310_ref.h"

#define DPS310_EVAL_CHANNEL_COUNT 2

static struct dps310_device g_eval_dev;
static int32_t g_cached[DPS310_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[DPS310_EVAL_CHANNEL_COUNT] = {
    {"temp",     "mC", 0},
    {"pressure", "Pa", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "dps310",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = DPS310_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "temp",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int dps310_eval_refresh_cache(void) {
    rt_int32_t temp_c100    = 0;
    rt_int32_t press_pa100  = 0;
    if (dps310_read_temperature(&g_eval_dev, &temp_c100) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (dps310_read_pressure(&g_eval_dev, &press_pa100) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = (int32_t)(temp_c100 * 10);       /* 0.01 C → mC */
    g_cached[1] = (int32_t)(press_pa100 / 100);    /* 0.01 Pa → Pa */
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (dps310_init(&g_eval_dev, bus_name, DPS310_DEFAULT_ADDR) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (dps310_probe(&g_eval_dev) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (dps310_read_calibration(&g_eval_dev) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= DPS310_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = dps310_eval_refresh_cache();
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
