#include "drivergen_eval_adapter.h"
#include "adxl345_ref.h"

#define ADXL345_EVAL_CHANNEL_COUNT 3

static struct adxl345_device g_dev;
static int16_t g_sample[ADXL345_EVAL_CHANNEL_COUNT];
static int g_sample_valid;

static const drivergen_eval_channel_t g_channels[ADXL345_EVAL_CHANNEL_COUNT] = {
    {"accel_x", "raw", 0},
    {"accel_y", "raw", 0},
    {"accel_z", "raw", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "adxl345",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = ADXL345_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "accel_x",
    .primary_unit       = "raw",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int adxl345_eval_refresh(void)
{
    struct adxl345_accel sample;
    if (adxl345_read_accel(&g_dev, &sample) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample[0] = sample.x;
    g_sample[1] = sample.y;
    g_sample[2] = sample.z;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    g_sample_valid = 0;
    return adxl345_init(&g_dev, 0, ADXL345_RANGE_2G) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out)
{
    if (out == 0) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id < 0 || channel_id >= ADXL345_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = adxl345_eval_refresh();
        if (err != DRIVERGEN_EVAL_OK) {
            return err;
        }
    }
    *out = (int32_t)g_sample[channel_id];
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
