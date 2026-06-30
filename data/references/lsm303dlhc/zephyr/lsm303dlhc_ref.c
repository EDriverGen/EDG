/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Accelerometer + Magnetometer Driver for Zephyr
 */
#include "lsm303dlhc_ref.h"
#include <errno.h>

int lsm303dlhc_init(struct lsm303dlhc_device *dev,
                    const struct device *bus)
{
  if (dev == NULL || bus == NULL) return -EINVAL;
  if (!device_is_ready(bus)) return -ENODEV;

  dev->bus = bus;
  dev->accel_addr = LSM303DLHC_ACCEL_ADDR;
  dev->mag_addr   = LSM303DLHC_MAG_ADDR;

  return 0;
}

int lsm303dlhc_probe(struct lsm303dlhc_device *dev)
{
  uint8_t ira, irb, irc;
  int ret;

  if (dev == NULL || dev->bus == NULL) return -EINVAL;

  ret = i2c_reg_read_byte(dev->bus, dev->mag_addr, LSM303_IRA_REG_M, &ira);
  if (ret < 0) return ret;

  ret = i2c_reg_read_byte(dev->bus, dev->mag_addr, LSM303_IRB_REG_M, &irb);
  if (ret < 0) return ret;

  ret = i2c_reg_read_byte(dev->bus, dev->mag_addr, LSM303_IRC_REG_M, &irc);
  if (ret < 0) return ret;

  if (ira != LSM303_IRA_VALUE || irb != LSM303_IRB_VALUE ||
      irc != LSM303_IRC_VALUE)
    {
      return -ENODEV;
    }

  return 0;
}

int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev)
{
  int ret;

  ret = i2c_reg_write_byte(dev->bus, dev->accel_addr,
                           LSM303_CTRL_REG1_A, LSM303_ACCEL_ODR_50HZ);
  if (ret < 0) return ret;

  return i2c_reg_write_byte(dev->bus, dev->accel_addr,
                            LSM303_CTRL_REG4_A, LSM303_ACCEL_FS_2G | 0x08);  /* HR bit */
}

int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev)
{
  int ret;

  ret = i2c_reg_write_byte(dev->bus, dev->mag_addr,
                           LSM303_CRA_REG_M, LSM303_MAG_ODR_15HZ);
  if (ret < 0) return ret;

  return i2c_reg_write_byte(dev->bus, dev->mag_addr,
                            LSM303_MR_REG_M, LSM303_MAG_CONTINUOUS);
}

int lsm303dlhc_read_accel(struct lsm303dlhc_device *dev,
                          struct lsm303dlhc_accel_data *data)
{
  uint8_t buf[6];
  int ret;

  if (dev == NULL || data == NULL) return -EINVAL;

  /* Auto-increment: set MSB of sub-address */
  ret = i2c_burst_read(dev->bus, dev->accel_addr,
                       LSM303_OUT_X_L_A | 0x80, buf, 6);
  if (ret < 0) return ret;

  /* 12-bit left-aligned, little-endian */
  data->x = (int16_t)((buf[1] << 8) | buf[0]) >> 4;
  data->y = (int16_t)((buf[3] << 8) | buf[2]) >> 4;
  data->z = (int16_t)((buf[5] << 8) | buf[4]) >> 4;

  return 0;
}

int lsm303dlhc_read_mag(struct lsm303dlhc_device *dev,
                        struct lsm303dlhc_mag_data *data)
{
  uint8_t buf[6];
  int ret;

  if (dev == NULL || data == NULL) return -EINVAL;

  ret = i2c_burst_read(dev->bus, dev->mag_addr,
                       LSM303_OUT_X_H_M, buf, 6);
  if (ret < 0) return ret;

  /* Big-endian, order: X_H, X_L, Z_H, Z_L, Y_H, Y_L */
  data->x = (int16_t)((buf[0] << 8) | buf[1]);
  data->z = (int16_t)((buf[2] << 8) | buf[3]);
  data->y = (int16_t)((buf[4] << 8) | buf[5]);

  return 0;
}
