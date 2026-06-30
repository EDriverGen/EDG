#include "drivergen_eval_adapter.h"
#include "bh1750_ref.h"

static struct bh1750_device g_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "bh1750",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "ambient_light",
    .primary_unit       = "lux_raw",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name)
{
    const char *path = (bus_name != NULL && bus_name[0] != '\0') ? bus_name : "/dev/i2c-0";
    return bh1750_init(&g_dev, path, BH1750_DEFAULT_ADDR) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out)
{
    uint16_t raw = 0;
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (bh1750_read_raw(&g_dev, &raw) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)raw;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    return DRIVERGEN_EVAL_OK;
}
