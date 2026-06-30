/* tmp421_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the TMP421 RT-Thread
 * reference driver (I2C multi_channel, 2 channels: temp_local, temp_remote).
 *
 * Reference driver API:
 *   rt_err_t tmp421_init(dev, const char *bus_name, rt_uint8_t addr);
 *   rt_err_t tmp421_probe(dev);
 *   rt_err_t tmp421_read_local_temp (dev, rt_int32_t *temp_mcelsius);
 *   rt_err_t tmp421_read_remote_temp(dev, rt_int32_t *temp_mcelsius);
 *
 * Units: driver returns mC directly (matches oracle's `mC` spec).
 */
#include "drivergen_eval_adapter.h"
#include "tmp421_ref.h"

#define TMP421_EVAL_CHANNEL_COUNT 2

static struct tmp421_device g_eval_dev;
static int32_t g_cached[TMP421_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[TMP421_EVAL_CHANNEL_COUNT] = {
    {"temp_local",  "mC", 0},
    {"temp_remote", "mC", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "tmp421",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = TMP421_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "temp_local",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int tmp421_eval_refresh_cache(void) {
    rt_int32_t local_mc = 0;
    rt_int32_t remote_mc = 0;
    if (tmp421_read_local_temp(&g_eval_dev, &local_mc) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (tmp421_read_remote_temp(&g_eval_dev, &remote_mc) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = (int32_t)local_mc;
    g_cached[1] = (int32_t)remote_mc;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (tmp421_init(&g_eval_dev, bus_name, TMP421_DEFAULT_ADDR) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (tmp421_probe(&g_eval_dev) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= TMP421_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = tmp421_eval_refresh_cache();
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
