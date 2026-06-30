#ifndef __VL53L0X_REF_H
#define __VL53L0X_REF_H
#include "tx_api.h"
#include <stdint.h>

#define VL53L0X_ADDR_DEFAULT  0x29
#define VL53L0X_MODEL_ID      0xEE


struct vl53l0x_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct vl53l0x_device {
  void *bus_context;
  const struct vl53l0x_i2c_ops *ops;
  uint16_t addr;
};

int vl53l0x_init(struct vl53l0x_device *dev, void *bus_context, const struct vl53l0x_i2c_ops *ops, uint16_t addr);
int vl53l0x_probe(struct vl53l0x_device *dev);
int vl53l0x_read_range_mm(struct vl53l0x_device *dev, uint16_t *range_mm);
#endif
