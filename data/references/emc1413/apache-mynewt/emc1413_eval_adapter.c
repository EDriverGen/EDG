#include "drivergen_eval_adapter.h"
#include "emc1413_ref.h"

#define EMC1413_EVAL_CHANNEL_COUNT 3

static struct emc1413_device g_dev;
static int32_t g_cached[EMC1413_EVAL_CHANNEL_COUNT];
static int g_sample_valid;

static const drivergen_eval_channel_t g_channels[EMC1413_EVAL_CHANNEL_COUNT] = {
    {"temp_local", "mC", 0},
    {"temp_ext1", "mC", 0},
    {"temp_ext2", "mC", 0},
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

static int emc1413_eval_refresh_cache(void)
{
    if (emc1413_read_temperature(&g_dev, EMC1413_CH_INTERNAL, &g_cached[0]) != 0 ||
        emc1413_read_temperature(&g_dev, EMC1413_CH_EXTERNAL_1, &g_cached[1]) != 0 ||
        emc1413_read_temperature(&g_dev, EMC1413_CH_EXTERNAL_2, &g_cached[2]) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    if (emc1413_init(&g_dev, 0, EMC1413_DEFAULT_ADDR) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (emc1413_probe(&g_dev) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out)
{
    if (out == NULL || channel_id < 0 || channel_id >= EMC1413_EVAL_CHANNEL_COUNT) {
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

int drivergen_eval_cleanup(void)
{
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
