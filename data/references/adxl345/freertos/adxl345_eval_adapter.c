/* adxl345_eval_adapter.c — Evaluation adapter for freertos */
#include "drivergen_eval_adapter.h"
#include "adxl345_ref.h"
#include "stm32f1xx_hal.h"

#define ADXL345_EVAL_CHANNEL_COUNT 3

static SPI_HandleTypeDef _hspi;

static struct adxl345_device g_eval_dev;

static int16_t g_sample[ADXL345_EVAL_CHANNEL_COUNT];
static int     g_sample_valid = 0;

static const drivergen_eval_channel_t g_channels[ADXL345_EVAL_CHANNEL_COUNT] = {
    {"accel_x", "lsb_raw_g", 0},
    {"accel_y", "lsb_raw_g", 0},
    {"accel_z", "lsb_raw_g", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "adxl345",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = ADXL345_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "accel_x",
    .primary_unit       = "lsb_raw_g",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int adxl345_eval_refresh_cache(void) {
    struct adxl345_accel a;
    if (adxl345_read_accel(&g_eval_dev, &a) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample[0] = a.x;
    g_sample[1] = a.y;
    g_sample[2] = a.z;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_SPI_Init(&_hspi);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    int err = adxl345_init(&g_eval_dev, &_hspi, GPIOA, GPIO_PIN_4, ADXL345_RANGE_2G);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= ADXL345_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = adxl345_eval_refresh_cache();
        if (err != DRIVERGEN_EVAL_OK) {
            return err;
        }
    }
    *out = (int32_t)g_sample[channel_id];
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
