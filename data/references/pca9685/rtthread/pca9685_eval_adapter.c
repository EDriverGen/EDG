#include "drivergen_eval_adapter.h"
#include "pca9685_ref.h"

static struct pca9685_device g_eval_dev;
static uint8_t g_cache[64];  /* 16 channels × 4 bytes */
static int g_cache_valid = 0;

/* Read all 16 channel registers (64 bytes from LED0_ON_L) in one go.
 * Uses auto-increment (AI=1 in MODE1). */
static int _refresh(void) {
    struct rt_i2c_msg msgs[2];
    uint8_t reg = PCA9685_REG_LED0_ON_L;
    msgs[0].addr  = g_eval_dev.addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;
    msgs[1].addr  = g_eval_dev.addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = g_cache;
    msgs[1].len   = 64;
    if (rt_i2c_transfer(g_eval_dev.bus, msgs, 2) != 2)
        return DRIVERGEN_EVAL_ERR_IO;
    g_cache_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    if (!bus_name) return DRIVERGEN_EVAL_ERR_INVALID;
    rt_device_t d = rt_device_find(bus_name);
    if (d == RT_NULL) return DRIVERGEN_EVAL_ERR_IO;
    return (pca9685_init(&g_eval_dev, (struct rt_i2c_bus_device *)d,
                         PCA9685_I2C_ADDR) == RT_EOK)
           ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
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
