#include "drivergen_eval_adapter.h"
#include "mcp23017_ref.h"
#include "stm32f1xx_hal.h"

static I2C_HandleTypeDef g_hi2c;
static uint8_t g_cache_a, g_cache_b;
static int g_cache_valid = 0;

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

static const struct mcp23017_i2c_ops g_i2c_ops = {
    .write_read = _tx_i2c_write_read,
};

static struct mcp23017_device g_eval_dev;

static int _refresh(void) {
    if (mcp23017_read_ports(&g_eval_dev, &g_cache_a, &g_cache_b) != 0)
        return DRIVERGEN_EVAL_ERR_IO;
    g_cache_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_I2C_Init(&g_hi2c);
    return (mcp23017_init(&g_eval_dev, &g_hi2c, &g_i2c_ops, MCP23017_I2C_ADDR) == 0)
           ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int idx, int32_t *val) {
    if (idx < 0 || idx > 15 || !val) return DRIVERGEN_EVAL_ERR_INVALID;
    if (!g_cache_valid) { int r = _refresh(); if (r) return r; }
    if (idx < 8) *val = (g_cache_a >> idx) & 1;
    else         *val = (g_cache_b >> (idx - 8)) & 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) { return DRIVERGEN_EVAL_OK; }

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id     = "mcp23017",
    .eval_class    = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count = 16,
    .channels = (const drivergen_eval_channel_t[]){
        {"gpa0","bool",0},{"gpa1","bool",0},{"gpa2","bool",0},{"gpa3","bool",0},
        {"gpa4","bool",0},{"gpa5","bool",0},{"gpa6","bool",0},{"gpa7","bool",0},
        {"gpb0","bool",0},{"gpb1","bool",0},{"gpb2","bool",0},{"gpb3","bool",0},
        {"gpb4","bool",0},{"gpb5","bool",0},{"gpb6","bool",0},{"gpb7","bool",0},
    },
    .primary_id    = "gpa0",
    .primary_unit  = "bool",
    .abi_version_major = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};
