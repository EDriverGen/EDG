/* ssd1306_eval_adapter.c — Evaluation adapter for threadx */
#include "drivergen_eval_adapter.h"
#include "ssd1306_ref.h"
#include "threadx.h"

#define SSD1306_EVAL_MAX_FRAME  1024u
#define SSD1306_EVAL_DATA_CTRL  0x40u  /* Co=0, D/C#=1 (pure data) */

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
static const struct ssd1306_i2c_ops _i2c_ops = {
    .write = _tx_i2c_write,
    .read = _tx_i2c_read,
    .write_read = _tx_i2c_write_read,
};

static struct ssd1306_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "ssd1306",
    .eval_class         = DRIVERGEN_EVAL_CLASS_DISPLAY,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = NULL,
    .primary_unit       = NULL,
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_I2C_Init(&_hi2c);
    int err = ssd1306_init(&g_eval_dev, &_hi2c, &_i2c_ops, SSD1306_I2C_ADDR);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}


int drivergen_eval_read_status(uint8_t *out) {
    (void)out;
    return DRIVERGEN_EVAL_ERR_UNSUPPORTED;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}

/* EVAL_DISPLAY_SHIM */
int drivergen_eval_output_frame(const uint8_t *data, uint16_t len) {
    (void)data; (void)len;
    return DRIVERGEN_EVAL_OK;
}
