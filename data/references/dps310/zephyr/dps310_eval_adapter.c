/* dps310_eval_adapter.c — Evaluation adapter for zephyr */
#include "drivergen_eval_adapter.h"
#include "dps310_ref.h"
#include "zephyr.h"

#define DPS310_EVAL_CHANNEL_COUNT 2

static struct device _i2c_dev = {.name = "i2c1"};

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
    /*
     * Zephyr driver returns temp in mC but pressure in 0.01 Pa (see
     * dps310_ref.h: temp_mcelsius, pressure_pa_x100). Eval ABI expects
     * {mC, Pa}, so temp passes through and pressure is scaled /100.
     */
    int32_t temp_mc       = 0;
    int32_t press_pa_x100 = 0;
    if (dps310_read_temperature(&g_eval_dev, &temp_mc) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (dps310_read_pressure(&g_eval_dev, &press_pa_x100) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = temp_mc;
    g_cached[1] = press_pa_x100 / 100;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    int err = dps310_init(&g_eval_dev, &_i2c_dev, DPS310_DEFAULT_ADDR);
    if (err != 0) return DRIVERGEN_EVAL_ERR_IO;
    if (dps310_probe(&g_eval_dev) != 0) return DRIVERGEN_EVAL_ERR_IO;
    if (dps310_read_calibration(&g_eval_dev) != 0) return DRIVERGEN_EVAL_ERR_IO;
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
