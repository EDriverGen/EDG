/* tmp421_eval_adapter.c — Evaluation adapter for openharmony */
#include "drivergen_eval_adapter.h"
#include "tmp421_ref.h"
#include "openharmony_liteosm.h"

#define TMP421_EVAL_CHANNEL_COUNT 2

static DevHandle _i2c_handle;

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
    int32_t local_mc = 0;
    int32_t remote_mc = 0;
    if (tmp421_read_local_temp(&g_eval_dev, &local_mc) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (tmp421_read_remote_temp(&g_eval_dev, &remote_mc) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = (int32_t)local_mc;
    g_cached[1] = (int32_t)remote_mc;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    _i2c_handle = I2cOpen(0);
    int err = tmp421_init(&g_eval_dev, _i2c_handle, TMP421_ADDR_DEFAULT);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
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
