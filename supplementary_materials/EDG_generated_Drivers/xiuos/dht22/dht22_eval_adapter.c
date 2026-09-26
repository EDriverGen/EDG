/* dht22_eval_adapter.c */
#include "drivergen_eval_adapter.h"
#include "dht22.h"

static struct dht22_device g_eval_dev;

static const drivergen_eval_channel_t g_channels[2] = {
    {"humidity", "milli_percent_rh", 0},
    {"temp", "milli_degC", 0}
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "dht22",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = 2,
    .channels           = g_channels,
    .primary_id         = "humidity",
    .primary_unit       = "milli_percent_rh",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int rc = dht22_init(&g_eval_dev);
    return (rc == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

static int32_t g_cached[2];
static int g_sample_valid = 0;

static int dht22_eval_refresh_cache(void) {
    int32_t humidity_val = 0;
    int32_t temp_val = 0;
    int rc = dht22_read_sensor(&g_eval_dev, &humidity_val, &temp_val);
    if (rc != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = humidity_val;
    g_cached[1] = temp_val;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= 2) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int rc = dht22_eval_refresh_cache();
        if (rc != DRIVERGEN_EVAL_OK) {
            return rc;
        }
    }
    *out = g_cached[channel_id];
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
