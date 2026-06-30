#include "drivergen_eval_adapter.h"
#include "pca9685_ref.h"

static struct pca9685_device g_eval_dev;
static DevHandle             g_i2c_handle;
static uint8_t               g_cache[64];
static int                   g_cache_valid = 0;

static int _i2c_write_read_internal(DevHandle bus, uint16_t addr,
                                    uint8_t *wdata, uint16_t wlen,
                                    uint8_t *rdata, uint16_t rlen)
{
    struct I2cMsg msg[2];
    uint8_t nmsg = 0;

    if (wlen > 0) {
        msg[nmsg].addr  = addr;
        msg[nmsg].flags = 0;
        msg[nmsg].buf   = wdata;
        msg[nmsg].len   = wlen;
        nmsg++;
    }
    if (rlen > 0) {
        msg[nmsg].addr  = addr;
        msg[nmsg].flags = I2C_FLAG_READ;
        msg[nmsg].buf   = rdata;
        msg[nmsg].len   = rlen;
        nmsg++;
    }
    return I2cTransfer(bus, msg, nmsg) == nmsg ? 0 : -1;
}

static int _refresh(void)
{
    uint8_t reg = PCA9685_REG_LED0_ON_L;
    int ret = _i2c_write_read_internal(g_eval_dev.bus, g_eval_dev.addr,
                                       &reg, 1, g_cache, 64);
    if (ret != 0) return DRIVERGEN_EVAL_ERR_IO;
    g_cache_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    g_i2c_handle = I2cOpen(0);
    if (g_i2c_handle == NULL) return DRIVERGEN_EVAL_ERR_IO;
    return (pca9685_init(&g_eval_dev, g_i2c_handle, PCA9685_I2C_ADDR) == 0)
           ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int idx, int32_t *val)
{
    uint8_t *ch;
    if (idx < 0 || idx > 15 || !val) return DRIVERGEN_EVAL_ERR_INVALID;
    if (!g_cache_valid) { int r = _refresh(); if (r) return r; }
    ch = &g_cache[idx * 4];
    if (ch[1] & PCA9685_LED_FULL_ON)      *val = 4096;
    else if (ch[3] & PCA9685_LED_FULL_OFF) *val = 0;
    else *val = (int32_t)(((ch[3] & 0x0F) << 8) | ch[2]);
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) { return DRIVERGEN_EVAL_OK; }

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id = "pca9685", .eval_class = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
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
    .primary_id = "led0", .primary_unit = "pwm_12bit",
    .abi_version_major = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};
