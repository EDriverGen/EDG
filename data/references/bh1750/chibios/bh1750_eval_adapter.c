/* bh1750_eval_adapter.c — Evaluation adapter for chibios */
#include "drivergen_eval_adapter.h"
#include "bh1750_ref.h"
#include "hal.h"

static struct bh1750_device g_eval_dev;

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

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    I2CConfig cfg = {0};
    i2cStart(&I2CD1, &cfg);
    int err = bh1750_init(&g_eval_dev, &I2CD1, BH1750_DEFAULT_ADDR);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    uint16_t raw_u16 = 0;
    int err = bh1750_read_raw(&g_eval_dev, &raw_u16);
    if (err != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)raw_u16;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
