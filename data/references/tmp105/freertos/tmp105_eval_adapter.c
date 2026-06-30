/* tmp105_eval_adapter.c — Evaluation adapter for freertos */
#include "drivergen_eval_adapter.h"
#include "tmp105_ref.h"
#include "stm32f1xx_hal.h"

static I2C_HandleTypeDef _hi2c;

static struct tmp105_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "tmp105",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "temp",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_I2C_Init(&_hi2c);
    int err = tmp105_init(&g_eval_dev, &_hi2c, TMP105_ADDR_DEFAULT);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int32_t mc = 0;
    int err = tmp105_read_temperature(&g_eval_dev, &mc);
    if (err != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)mc;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
