/* sht30_eval_adapter.c — Evaluation adapter for xiuos */
#include "drivergen_eval_adapter.h"
#include "sht30_ref.h"
#include "xiuos.h"

#define SHT30_EVAL_CHANNEL_COUNT 2

static struct sht30_device g_eval_dev;
static int32_t g_cached[SHT30_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[SHT30_EVAL_CHANNEL_COUNT] = {
    {"temp",     "mC",           0},
    {"humidity", "pctRH_x1000",  0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "sht30",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = SHT30_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "temp",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int sht30_eval_refresh_cache(void) {
    int32_t temp_mc  = 0;
    int32_t rh_mperc = 0;
    if (sht30_read(&g_eval_dev, &temp_mc, &rh_mperc) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = temp_mc;
    g_cached[1] = rh_mperc;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    int err = sht30_init(&g_eval_dev, bus_name, SHT30_ADDR_DEFAULT);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= SHT30_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = sht30_eval_refresh_cache();
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
