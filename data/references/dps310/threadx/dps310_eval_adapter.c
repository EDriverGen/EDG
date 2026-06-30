/* dps310_eval_adapter.c — Evaluation adapter for threadx */
#include "drivergen_eval_adapter.h"
#include "dps310_ref.h"
#include "threadx.h"

#define DPS310_EVAL_CHANNEL_COUNT 2

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
static const struct dps310_i2c_ops _i2c_ops = {
    .write = _tx_i2c_write,
    .read = _tx_i2c_read,
    .write_read = _tx_i2c_write_read,
};

static struct dps310_device g_eval_dev;
static int32_t g_cached[DPS310_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[DPS310_EVAL_CHANNEL_COUNT] = {
    {"temp",     "mC", 0},
    {"pressure", "Pa", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "dps310",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = DPS310_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "temp",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int dps310_eval_refresh_cache(void) {
    /*
     * ThreadX driver returns mC for temp and Pa for pressure, matching
     * the eval ABI channel units; no scaling. (rtthread's adapter
     * multiplies/divides by 100 because its driver returns cC/cPa.)
     */
    int32_t temp_mc  = 0;
    int32_t press_pa = 0;
    if (dps310_read_temperature(&g_eval_dev, &temp_mc) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    if (dps310_read_pressure(&g_eval_dev, &press_pa) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_cached[0] = temp_mc;
    g_cached[1] = press_pa;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_I2C_Init(&_hi2c);
    int err = dps310_init(&g_eval_dev, &_hi2c, &_i2c_ops, DPS310_DEFAULT_ADDR);
    if (err != 0) return DRIVERGEN_EVAL_ERR_IO;
    if (dps310_probe(&g_eval_dev) != 0) return DRIVERGEN_EVAL_ERR_IO;
    if (dps310_read_calibration(&g_eval_dev) != 0) return DRIVERGEN_EVAL_ERR_IO;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= DPS310_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = dps310_eval_refresh_cache();
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
