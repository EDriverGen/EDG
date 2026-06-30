#include "drivergen_eval_adapter.h"
#include "mpu6050_ref.h"

#define MPU6050_EVAL_CHANNEL_COUNT 6

static struct mpu6050_device g_dev;
static int16_t g_sample[MPU6050_EVAL_CHANNEL_COUNT];
static int g_sample_valid;

static const drivergen_eval_channel_t g_channels[MPU6050_EVAL_CHANNEL_COUNT] = {
    {"accel_x", "lsb_raw_g", 0},
    {"accel_y", "lsb_raw_g", 0},
    {"accel_z", "lsb_raw_g", 0},
    {"gyro_x", "lsb_raw_dps", 0},
    {"gyro_y", "lsb_raw_dps", 0},
    {"gyro_z", "lsb_raw_dps", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "mpu6050",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = MPU6050_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "accel_x",
    .primary_unit       = "lsb_raw_g",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int mpu6050_eval_refresh_cache(void)
{
    int16_t ax, ay, az, gx, gy, gz;
    if (mpu6050_read_accel(&g_dev, &ax, &ay, &az) != 0 ||
        mpu6050_read_gyro(&g_dev, &gx, &gy, &gz) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample[0] = ax; g_sample[1] = ay; g_sample[2] = az;
    g_sample[3] = gx; g_sample[4] = gy; g_sample[5] = gz;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name)
{
    const char *path = (bus_name != NULL && bus_name[0] != '\0') ? bus_name : "/dev/i2c-0";
    if (mpu6050_init(&g_dev, path, MPU6050_ADDR_DEFAULT) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out)
{
    if (out == NULL || channel_id < 0 || channel_id >= MPU6050_EVAL_CHANNEL_COUNT) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (channel_id == 0 || !g_sample_valid) {
        int err = mpu6050_eval_refresh_cache();
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
