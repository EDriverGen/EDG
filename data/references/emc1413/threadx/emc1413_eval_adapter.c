/* emc1413_eval_adapter.c — Evaluation adapter for threadx */
#include "drivergen_eval_adapter.h"
#include "emc1413_ref.h"
#include "threadx.h"

#define EMC1413_EVAL_CHANNEL_COUNT 3

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
static const struct emc1413_i2c_ops _i2c_ops = {
    .write = _tx_i2c_write,
    .read = _tx_i2c_read,
    .write_read = _tx_i2c_write_read,
};

static struct emc1413_device g_eval_dev;
static int32_t g_cached[EMC1413_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[EMC1413_EVAL_CHANNEL_COUNT] = {
    {"temp_local", "mC", 0},
    {"temp_ext1",  "mC", 0},
    {"temp_ext2",  "mC", 0},
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

static int emc1413_eval_refresh_cache(void) {
    enum emc1413_channel order[EMC1413_EVAL_CHANNEL_COUNT] = {
        EMC1413_CH_INTERNAL,
        EMC1413_CH_EXTERNAL_1,
        EMC1413_CH_EXTERNAL_2,
    };
    for (int i = 0; i < EMC1413_EVAL_CHANNEL_COUNT; i++) {
        int32_t mc = 0;
        if (emc1413_read_temperature(&g_eval_dev, order[i], &mc) != 0) {
            return DRIVERGEN_EVAL_ERR_IO;
        }
        g_cached[i] = (int32_t)mc;
    }
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_I2C_Init(&_hi2c);
    int err = emc1413_init(&g_eval_dev, &_hi2c, &_i2c_ops, EMC1413_ADDR_DEFAULT);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= EMC1413_EVAL_CHANNEL_COUNT) {
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

int drivergen_eval_cleanup(void) {
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
