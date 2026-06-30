/* lm75a_eval_adapter.c — Evaluation adapter for tobudos */
#include "drivergen_eval_adapter.h"
#include "lm75a_ref.h"
#include "tobudos.h"

static I2C_HandleTypeDef _hi2c;

static struct lm75a_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "lm75a",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "temp",
    .primary_unit       = "eighth_celsius",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_I2C_Init(&_hi2c);
    int err = lm75a_init(&g_eval_dev, &_hi2c, LM75A_DEFAULT_ADDR);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int16_t raw_i16 = 0;
    int err = lm75a_read_raw(&g_eval_dev, &raw_i16);
    if (err != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)raw_i16;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
