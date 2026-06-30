/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Accelerometer + Magnetometer Driver for XiUOS
 *
 * Note: LSM303DLHC has two I2C addresses (accel=0x19, mag=0x1E),
 * so we open the I2C device path twice with different slave addresses.
 */
#include "lsm303dlhc_ref.h"

static int lsm303_open_addr(const char *i2c_dev_path, uint16_t addr)
{
  struct PrivIoctlCfg ioctl_cfg;
  uint16_t i2c_addr = addr;
  int fd;

  fd = PrivOpen(i2c_dev_path, O_RDWR);
  if (fd < 0) return -1;

  ioctl_cfg.ioctl_driver_type = I2C_TYPE;
  ioctl_cfg.args = &i2c_addr;
  if (PrivIoctl(fd, OPE_INT, &ioctl_cfg) < 0)
    {
      PrivClose(fd);
      return -1;
    }

  return fd;
}

static int lsm303_read_reg(int fd, uint8_t reg, uint8_t *buf, int len)
{
  /* Set auto-increment bit for multi-byte reads */
  uint8_t addr = (len > 1) ? (reg | 0x80) : reg;

  if (PrivWrite(fd, &addr, 1) < 0) return -1;
  if (PrivRead(fd, buf, len) < 0) return -1;
  return 0;
}

static int lsm303_write_reg(int fd, uint8_t reg, uint8_t value)
{
  uint8_t frame[2];

  frame[0] = reg;
  frame[1] = value;
  if (PrivWrite(fd, frame, 2) < 0) return -1;
  return 0;
}

int lsm303dlhc_init(struct lsm303dlhc_device *dev,
                    const char *i2c_dev_path)
{
  if (dev == NULL || i2c_dev_path == NULL) return -1;

  dev->accel_fd = lsm303_open_addr(i2c_dev_path, LSM303DLHC_ACCEL_ADDR);
  if (dev->accel_fd < 0) return -1;

  dev->mag_fd = lsm303_open_addr(i2c_dev_path, LSM303DLHC_MAG_ADDR);
  if (dev->mag_fd < 0)
    {
      PrivClose(dev->accel_fd);
      dev->accel_fd = -1;
      return -1;
    }

  return 0;
}

void lsm303dlhc_deinit(struct lsm303dlhc_device *dev)
{
  if (dev == NULL) return;
  if (dev->accel_fd >= 0) { PrivClose(dev->accel_fd); dev->accel_fd = -1; }
  if (dev->mag_fd >= 0)   { PrivClose(dev->mag_fd);   dev->mag_fd = -1; }
}

int lsm303dlhc_probe(struct lsm303dlhc_device *dev)
{
  uint8_t ira, irb, irc;

  if (dev == NULL) return -1;

  if (lsm303_read_reg(dev->mag_fd, LSM303_IRA_REG_M, &ira, 1) < 0) return -1;
  if (lsm303_read_reg(dev->mag_fd, LSM303_IRB_REG_M, &irb, 1) < 0) return -1;
  if (lsm303_read_reg(dev->mag_fd, LSM303_IRC_REG_M, &irc, 1) < 0) return -1;

  if (ira != LSM303_IRA_VALUE || irb != LSM303_IRB_VALUE ||
      irc != LSM303_IRC_VALUE)
    return -1;

  return 0;
}

int lsm303dlhc_accel_start(struct lsm303dlhc_device *dev)
{
  if (lsm303_write_reg(dev->accel_fd, LSM303_CTRL_REG1_A, LSM303_ACCEL_ODR_50HZ) < 0)
    return -1;

  return lsm303_write_reg(dev->accel_fd, LSM303_CTRL_REG4_A, LSM303_ACCEL_FS_2G | 0x08);  /* HR bit */
}

int lsm303dlhc_mag_start(struct lsm303dlhc_device *dev)
{
  if (lsm303_write_reg(dev->mag_fd, LSM303_CRA_REG_M, LSM303_MAG_ODR_15HZ) < 0)
    return -1;

  return lsm303_write_reg(dev->mag_fd, LSM303_MR_REG_M, LSM303_MAG_CONTINUOUS);
}

int lsm303dlhc_read_accel(struct lsm303dlhc_device *dev,
                          struct lsm303dlhc_accel_data *data)
{
  uint8_t buf[6];

  if (dev == NULL || data == NULL) return -1;

  if (lsm303_read_reg(dev->accel_fd, LSM303_OUT_X_L_A, buf, 6) < 0) return -1;

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

  if (dev == NULL || data == NULL) return -1;

  if (lsm303_read_reg(dev->mag_fd, LSM303_OUT_X_H_M, buf, 6) < 0) return -1;

  /* Big-endian, X-Z-Y order */
  data->x = (int16_t)((buf[0] << 8) | buf[1]);
  data->z = (int16_t)((buf[2] << 8) | buf[3]);
  data->y = (int16_t)((buf[4] << 8) | buf[5]);

  return 0;
}
