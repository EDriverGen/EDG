#include "drivergen_eval_adapter.h"
#include "dps310_ref.h"

#define DPS310_EVAL_CHANNEL_COUNT 2

static I2C_HandleTypeDef g_i2c;
static struct dps310_device g_dev;
static int32_t g_cached[DPS310_EVAL_CHANNEL_COUNT];
static int g_sample_valid;

static const drivergen_eval_channel_t g_channels[DPS310_EVAL_CHANNEL_COUNT] = {
    {"temp", "mC", 0},
    {"pressure", "Pa", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id = "dps310",
    .eval_class = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count = DPS310_EVAL_CHANNEL_COUNT,
    .channels = g_channels,
    .primary_id = "temp",
    .primary_unit = "mC",
    .memory_size_bytes = 0,
    .memory_page_bytes = 0,
    .abi_version_major = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int dps310_eval_refresh_cache(void)
{
    int32_t temp_c100 = 0;
    int32_t press_pa100 = 0;
    if (dps310_read_temperature(&g_dev, &temp_c100) != 0) return DRIVERGEN_EVAL_ERR_IO;
    if (dps310_read_pressure(&g_dev, &press_pa100) != 0) return DRIVERGEN_EVAL_ERR_IO;
    g_cached[0] = temp_c100 * 10;
    g_cached[1] = press_pa100 / 100;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    if (dps310_init(&g_dev, &g_i2c, DPS310_DEFAULT_ADDR) != 0 ||
        dps310_probe(&g_dev) != 0 ||
        dps310_read_calibration(&g_dev) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out)
{
    if (out == NULL || channel_id < 0 || channel_id >= DPS310_EVAL_CHANNEL_COUNT) return DRIVERGEN_EVAL_ERR_INVALID;
    if (channel_id == 0 || !g_sample_valid) {
        int err = dps310_eval_refresh_cache();
        if (err != DRIVERGEN_EVAL_OK) return err;
    }
    *out = g_cached[channel_id];
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
