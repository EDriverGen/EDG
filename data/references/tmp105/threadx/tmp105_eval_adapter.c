/* tmp105_eval_adapter.c — Evaluation adapter for threadx */
#include "drivergen_eval_adapter.h"
#include "tmp105_ref.h"
#include "threadx.h"

static I2C_HandleTypeDef _hi2c;

static int _tx_i2c_write(void *ctx, uint16_t addr, const uint8_t *data, uint16_t len) {
    return HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)ctx, (uint16_t)(addr << 1),
                                   (uint8_t *)data, len, 100) == HAL_OK ? 0 : -1;
}
static int _tx_i2c_read(void *ctx, uint16_t addr, uint8_t *data, uint16_t len) {
    return HAL_I2C_Master_Receive((I2C_HandleTypeDef *)ctx, (uint16_t)(addr << 1),
                                  data, len, 100) == HAL_OK ? 0 : -1;
}
static int _tx_i2c_write_read(void *ctx, uint16_t addr,
                              const uint8_t *wd, uint16_t wl,
                              uint8_t *rd, uint16_t rl) {
    if (wl == 1) {
        return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)ctx, (uint16_t)(addr << 1),
                                wd[0], I2C_MEMADD_SIZE_8BIT, rd, rl, 100) == HAL_OK ? 0 : -1;
    }
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)ctx, (uint16_t)(addr << 1),
                                 (uint8_t *)wd, wl, 100) != HAL_OK) return -1;
    return HAL_I2C_Master_Receive((I2C_HandleTypeDef *)ctx, (uint16_t)(addr << 1),
                                   rd, rl, 100) == HAL_OK ? 0 : -1;
}
static const struct tmp105_i2c_ops _i2c_ops = {
    .write = _tx_i2c_write,
    .read = _tx_i2c_read,
    .write_read = _tx_i2c_write_read,
};

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
    int err = tmp105_init(&g_eval_dev, &_hi2c, &_i2c_ops, TMP105_ADDR_DEFAULT);
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
