/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Accelerometer + Magnetometer Driver for NuttX
 */
#include "lsm303dlhc_ref.h"
#include <errno.h>
#include <string.h>

/* ---- Internal helpers ---- */

static int lsm303_accel_read(FAR struct lsm303dlhc_device *dev,
                             uint8_t reg, FAR uint8_t *buf, int len)
{
  /* Auto-increment bit (bit 7) for multi-byte reads */
  uint8_t addr = reg | 0x80;

  return i2c_writeread(dev->i2c, &dev->accel_config, &addr, 1, buf, len);
}

static int lsm303_accel_write(FAR struct lsm303dlhc_device *dev,
                              uint8_t reg, uint8_t value)
{
  uint8_t frame[2];

  frame[0] = reg;
  frame[1] = value;
  return i2c_write(dev->i2c, &dev->accel_config, frame, 2);
}

static int lsm303_mag_read(FAR struct lsm303dlhc_device *dev,
                           uint8_t reg, FAR uint8_t *buf, int len)
{
  return i2c_writeread(dev->i2c, &dev->mag_config, &reg, 1, buf, len);
}

static int lsm303_mag_write(FAR struct lsm303dlhc_device *dev,
                            uint8_t reg, uint8_t value)
{
  uint8_t frame[2];

  frame[0] = reg;
  frame[1] = value;
  return i2c_write(dev->i2c, &dev->mag_config, frame, 2);
}

/* ---- Public API ---- */

int lsm303dlhc_init(FAR struct lsm303dlhc_device *dev,
                    FAR struct i2c_master_s *i2c)
{
  if (dev == NULL || i2c == NULL)
    {
      return -EINVAL;
    }

  dev->i2c = i2c;

  dev->accel_config.frequency = LSM303DLHC_I2C_FREQ;
  dev->accel_config.address   = LSM303DLHC_ACCEL_ADDR;
  dev->accel_config.addrlen   = 7;

  dev->mag_config.frequency = LSM303DLHC_I2C_FREQ;
  dev->mag_config.address   = LSM303DLHC_MAG_ADDR;
  dev->mag_config.addrlen   = 7;

  return 0;
}

int lsm303dlhc_probe(FAR struct lsm303dlhc_device *dev)
{
  uint8_t ira, irb, irc;
  int ret;

  if (dev == NULL || dev->i2c == NULL)
    {
      return -EINVAL;
    }

  /* Check magnetometer identification registers */
  ret = lsm303_mag_read(dev, LSM303_IRA_REG_M, &ira, 1);
  if (ret < 0) return ret;

  ret = lsm303_mag_read(dev, LSM303_IRB_REG_M, &irb, 1);
  if (ret < 0) return ret;

  ret = lsm303_mag_read(dev, LSM303_IRC_REG_M, &irc, 1);
  if (ret < 0) return ret;

  if (ira != LSM303_IRA_VALUE || irb != LSM303_IRB_VALUE ||
      irc != LSM303_IRC_VALUE)
    {
      return -ENODEV;
    }

  return 0;
}

int lsm303dlhc_accel_start(FAR struct lsm303dlhc_device *dev)
{
  int ret;

  ret = lsm303_accel_write(dev, LSM303_CTRL_REG1_A, LSM303_ACCEL_ODR_50HZ);
  if (ret < 0) return ret;

  ret = lsm303_accel_write(dev, LSM303_CTRL_REG4_A, LSM303_ACCEL_FS_2G | 0x08);  /* HR bit */
  return ret;
}

int lsm303dlhc_mag_start(FAR struct lsm303dlhc_device *dev)
{
  int ret;

  ret = lsm303_mag_write(dev, LSM303_CRA_REG_M, LSM303_MAG_ODR_15HZ);
  if (ret < 0) return ret;

  ret = lsm303_mag_write(dev, LSM303_MR_REG_M, LSM303_MAG_CONTINUOUS);
  return ret;
}

int lsm303dlhc_read_accel(FAR struct lsm303dlhc_device *dev,
                          FAR struct lsm303dlhc_accel_data *data)
{
  uint8_t buf[6];
  int ret;

  if (dev == NULL || data == NULL)
    {
      return -EINVAL;
    }

  /* Read 6 bytes starting from OUT_X_L_A with auto-increment */
  ret = lsm303_accel_read(dev, LSM303_OUT_X_L_A, buf, 6);
  if (ret < 0) return ret;

  /*
   * Accelerometer data is 12-bit, left-aligned in 16 bits.
   * Little-endian: low byte first, then high byte.
   */
  data->x = (int16_t)((buf[1] << 8) | buf[0]) >> 4;
  data->y = (int16_t)((buf[3] << 8) | buf[2]) >> 4;
  data->z = (int16_t)((buf[5] << 8) | buf[4]) >> 4;

  return 0;
}

int lsm303dlhc_read_mag(FAR struct lsm303dlhc_device *dev,
                        FAR struct lsm303dlhc_mag_data *data)
{
  uint8_t buf[6];
  int ret;

  if (dev == NULL || data == NULL)
    {
      return -EINVAL;
    }

  ret = lsm303_mag_read(dev, LSM303_OUT_X_H_M, buf, 6);
  if (ret < 0) return ret;

  /*
   * Magnetometer output in big-endian order: X_H, X_L, Z_H, Z_L, Y_H, Y_L
   */
  data->x = (int16_t)((buf[0] << 8) | buf[1]);
  data->z = (int16_t)((buf[2] << 8) | buf[3]);
  data->y = (int16_t)((buf[4] << 8) | buf[5]);

  return 0;
}
