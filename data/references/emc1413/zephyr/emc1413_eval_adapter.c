/* emc1413_eval_adapter.c — Evaluation adapter for zephyr */
#include "drivergen_eval_adapter.h"
#include "emc1413_ref.h"
#include "zephyr.h"

#define EMC1413_EVAL_CHANNEL_COUNT 3

static struct device _i2c_dev = {.name = "i2c1"};

static struct emc1413_device g_eval_dev;
static int32_t g_cached[EMC1413_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[EMC1413_EVAL_CHANNEL_COUNT] = {
    {"temp_local", "mC", 0},
    {"temp_ext1",  "mC", 0},
    {"temp_ext2",  "mC", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "emc1413",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = EMC1413_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "temp_local",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int emc1413_eval_refresh_cache(void) {
    enum emc1413_channel order[EMC1413_EVAL_CHANNEL_COUNT] = {
        EMC1413_CH_INTERNAL,
        EMC1413_CH_EXTERNAL_1,
        EMC1413_CH_EXTERNAL_2,
    };
    for (int i = 0; i < EMC1413_EVAL_CHANNEL_COUNT; i++) {
        int32_t mc = 0;
        if (emc1413_read_temperature(&g_eval_dev, order[i], &mc) != 0) {
            return DRIVERGEN_EVAL_ERR_IO;
        }
        g_cached[i] = (int32_t)mc;
    }
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    int err = emc1413_init(&g_eval_dev, &_i2c_dev, EMC1413_ADDR_DEFAULT);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= EMC1413_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = emc1413_eval_refresh_cache();
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
