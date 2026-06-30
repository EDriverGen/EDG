/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Driver for NuttX
 */
#ifndef __TMP421_REF_H
#define __TMP421_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define TMP421_ADDR_DEFAULT          0x2A
#define TMP421_I2C_FREQ              100000

/* Register Map */
#define TMP421_REG_LOCAL_TEMP_HI     0x00
#define TMP421_REG_LOCAL_TEMP_LO     0x10
#define TMP421_REG_REMOTE_TEMP_HI    0x01
#define TMP421_REG_REMOTE_TEMP_LO    0x11
#define TMP421_REG_STATUS            0x08
#define TMP421_REG_CONFIG_1          0x09
#define TMP421_REG_CONFIG_2          0x0A
#define TMP421_REG_CONV_RATE_RD      0x04
#define TMP421_REG_CONV_RATE_WR      0x0B
#define TMP421_REG_ONE_SHOT          0x0F
#define TMP421_REG_MANUFACTURER_ID   0xFE
#define TMP421_REG_DEVICE_ID         0xFF

#define TMP421_MANUFACTURER_ID_TI    0x55
#define TMP421_CONFIG1_RANGE         (1U << 2)
#define TMP421_CONFIG1_SHUTDOWN      (1U << 6)

struct tmp421_device
{
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
};

int tmp421_init(FAR struct tmp421_device *dev,
                FAR struct i2c_master_s *i2c,
                uint8_t addr);
int tmp421_probe(FAR struct tmp421_device *dev);
int tmp421_read_local_temp(FAR struct tmp421_device *dev,
                           FAR int32_t *temp_mcelsius);
int tmp421_read_remote_temp(FAR struct tmp421_device *dev,
                            FAR int32_t *temp_mcelsius);
int tmp421_set_extended_range(FAR struct tmp421_device *dev, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* __TMP421_REF_H */
