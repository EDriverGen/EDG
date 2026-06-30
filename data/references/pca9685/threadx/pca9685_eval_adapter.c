/*
 * pca9685_eval_adapter.c — Evaluation adapter for threadx
 */
#include "drivergen_eval_adapter.h"
#include "pca9685_ref.h"
#include "stm32f1xx_hal.h"

static I2C_HandleTypeDef g_hi2c;
static uint8_t g_cache[64];  /* 16 channels x 4 bytes */
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
    if (rl == 0) return 0;
    return HAL_I2C_Master_Receive((I2C_HandleTypeDef *)ctx, (uint16_t)(addr << 1),
                                   rd, rl, 100) == HAL_OK ? 0 : -1;
}

static const struct pca9685_i2c_ops g_i2c_ops = {
    .write_read = _tx_i2c_write_read,
};

static struct pca9685_device g_eval_dev;

/* Read all 16 channel registers (64 bytes from LED0_ON_L) in one go.
 * Uses auto-increment (AI=1 in MODE1). */
static int _refresh(void) {
    uint8_t reg = PCA9685_REG_LED0_ON_L;
    if (g_i2c_ops.write_read(g_eval_dev.bus_context, g_eval_dev.addr,
                              &reg, 1, g_cache, 64) != 0)
        return DRIVERGEN_EVAL_ERR_IO;
    g_cache_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_I2C_Init(&g_hi2c);
    int err = pca9685_init(&g_eval_dev, &g_hi2c, &g_i2c_ops, PCA9685_I2C_ADDR);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int idx, int32_t *val) {
    uint16_t on, off;
    uint8_t *ch;

    if (idx < 0 || idx > 15 || !val) return DRIVERGEN_EVAL_ERR_INVALID;
    if (!g_cache_valid) { int r = _refresh(); if (r) return r; }

    ch = &g_cache[idx * 4];
    on  = (uint16_t)((ch[1] & 0x0F) << 8) | ch[0];
    off = (uint16_t)((ch[3] & 0x0F) << 8) | ch[2];

    if (ch[1] & PCA9685_LED_FULL_ON)
        *val = 4096;
    else if (ch[3] & PCA9685_LED_FULL_OFF)
        *val = 0;
    else
        *val = (int32_t)(off & 0x0FFF);

    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) { return DRIVERGEN_EVAL_OK; }

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id     = "pca9685",
    .eval_class    = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count = 16,
    .channels = (const drivergen_eval_channel_t[]){
        {"led0","pwm_12bit",0},{"led1","pwm_12bit",0},
        {"led2","pwm_12bit",0},{"led3","pwm_12bit",0},
        {"led4","pwm_12bit",0},{"led5","pwm_12bit",0},
        {"led6","pwm_12bit",0},{"led7","pwm_12bit",0},
        {"led8","pwm_12bit",0},{"led9","pwm_12bit",0},
        {"led10","pwm_12bit",0},{"led11","pwm_12bit",0},
        {"led12","pwm_12bit",0},{"led13","pwm_12bit",0},
        {"led14","pwm_12bit",0},{"led15","pwm_12bit",0},
    },
    .primary_id    = "led0",
    .primary_unit  = "pwm_12bit",
    .abi_version_major = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};
