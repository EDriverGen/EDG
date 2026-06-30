#include "drivergen_eval_adapter.h"
#include "max31855_ref.h"

static struct max31855_device g_dev;
static uint32_t g_raw;
static int g_raw_valid;

static const drivergen_eval_channel_t g_channels[] = {
    {"thermocouple", "mC", 0},
    {"temp_local", "mC", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "max31855",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = 2,
    .channels           = g_channels,
    .primary_id         = "temp_thermocouple",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    g_raw_valid = 0;
    return max31855_init(&g_dev, 0) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

static int max31855_eval_refresh(void)
{
    if (!g_raw_valid) {
        if (max31855_read_raw(&g_dev, &g_raw) != 0) {
            return DRIVERGEN_EVAL_ERR_IO;
        }
        if (max31855_has_fault(g_raw)) {
            return DRIVERGEN_EVAL_ERR_IO;
        }
        g_raw_valid = 1;
    }
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out)
{
    int32_t mc = 0;
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= 2) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (max31855_eval_refresh() != DRIVERGEN_EVAL_OK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (channel_id == 0) {
        if (max31855_get_thermocouple_temp(g_raw, &mc) != 0) {
            return DRIVERGEN_EVAL_ERR_IO;
        }
    } else if (max31855_get_internal_temp(g_raw, &mc) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = mc;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    g_raw_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
