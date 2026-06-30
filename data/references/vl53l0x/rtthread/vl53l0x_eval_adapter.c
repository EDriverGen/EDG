/* vl53l0x_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the VL53L0X
 * RT-Thread reference driver (I2C single_channel).
 *
 * Reference driver API:
 *   int vl53l0x_init(struct vl53l0x_device *dev,
 *                    struct rt_i2c_bus_device *bus, uint16_t addr);
 *   int vl53l0x_probe(struct vl53l0x_device *dev);
 *   int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm);
 *
 * The driver expects `rt_i2c_bus_device*` (not a bus-name string), so
 * the adapter resolves the bus via `rt_device_find` and casts. Return
 * value is a u16 distance in mm, zero-extended into the int32 ABI.
 */
#include "drivergen_eval_adapter.h"
#include "vl53l0x_ref.h"

static struct vl53l0x_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "vl53l0x",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "distance",
    .primary_unit       = "mm",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

#define VL53L0X_DEFAULT_ADDR  0x29

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_device_t dev = rt_device_find(bus_name);
    if (dev == RT_NULL) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)dev;
    if (vl53l0x_init(&g_eval_dev, bus, VL53L0X_DEFAULT_ADDR) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    uint16_t mm = 0;
    if (vl53l0x_read_range_mm(&g_eval_dev, &mm) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)mm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
