/* adxl345_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the ADXL345 RT-Thread
 * reference driver (SPI bus). Provides the minimal eval ABI surface for
 * the "multi_channel" eval_class (3 channels: accel_x, accel_y, accel_z).
 *
 * Reference driver API:
 *   rt_err_t adxl345_init(dev, const char *device_name, uint8_t range);
 *   rt_err_t adxl345_read_accel(dev, struct adxl345_accel *accel);
 *   rt_err_t adxl345_read_accel_mg(dev, &x_mg, &y_mg, &z_mg);
 *
 * `bus_name` (e.g. "spi1") is passed through to `adxl345_init`
 * unchanged; the reference driver internally calls `rt_device_find` to
 * resolve it to a `struct rt_spi_device *`.
 *
 * Channel order (must stay in sync with the oracle meta.channels[] list):
 *   0: accel_x   1: accel_y   2: accel_z
 *
 * Range is fixed at +-2 g (ADXL345_RANGE_2G) for the evaluation baseline
 * so stimuli can assume a known LSB/g scaling. Per the ADXL345 datasheet
 * with full-resolution bit set, 1 mg / 4 ≈ 3.9 mg/LSB regardless of
 * range; `raw` values therefore follow a single scale across stimuli.
 */
#include "drivergen_eval_adapter.h"
#include "adxl345_ref.h"

#define ADXL345_EVAL_CHANNEL_COUNT 3

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
    if (adxl345_read_accel(&g_eval_dev, &a) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample[0] = a.x;
    g_sample[1] = a.y;
    g_sample[2] = a.z;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_err_t err = adxl345_init(&g_eval_dev, bus_name, ADXL345_RANGE_2G);
    if (err != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
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
