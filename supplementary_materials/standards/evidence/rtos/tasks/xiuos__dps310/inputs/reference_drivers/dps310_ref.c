/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Digital Pressure Sensor Driver for XiUOS
 */
#include "dps310_ref.h"
#include <string.h>

static int dps310_read_reg(struct dps310_device *dev,
                           uint8_t reg, uint8_t *buf, int len)
{
  if (PrivWrite(dev->fd, &reg, 1) < 0) return -1;
  if (PrivRead(dev->fd, buf, len) < 0) return -1;
  return 0;
}

static int dps310_write_reg(struct dps310_device *dev,
                            uint8_t reg, uint8_t value)
{
  uint8_t frame[2];

  frame[0] = reg;
  frame[1] = value;
  if (PrivWrite(dev->fd, frame, 2) < 0) return -1;
  return 0;
}

static int32_t dps310_twos_complement(uint32_t val, uint8_t bits)
{
  if (val & ((uint32_t)1 << (bits - 1)))
    return (int32_t)(val | (~(uint32_t)0 << bits));
  return (int32_t)val;
}

static int dps310_wait_ready(struct dps310_device *dev,
                             uint8_t mask, int timeout_ms)
{
  uint8_t meas_cfg;
  int elapsed = 0;

  while (elapsed < timeout_ms)
    {
      if (dps310_read_reg(dev, DPS310_REG_MEAS_CFG, &meas_cfg, 1) < 0)
        return -1;

      if (meas_cfg & mask) return 0;

      PrivTaskDelay(10);
      elapsed += 10;
    }

  return -1;
}

int dps310_init(struct dps310_device *dev,
                const char *i2c_dev_path,
                uint16_t addr)
{
  struct PrivIoctlCfg ioctl_cfg;
  uint16_t i2c_addr = addr;

  if (dev == NULL || i2c_dev_path == NULL) return -1;

  memset(dev, 0, sizeof(*dev));

  dev->fd = PrivOpen(i2c_dev_path, O_RDWR);
  if (dev->fd < 0) return -1;

  ioctl_cfg.ioctl_driver_type = I2C_TYPE;
  ioctl_cfg.args = &i2c_addr;
  if (PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg) < 0)
    {
      PrivClose(dev->fd);
      dev->fd = -1;
      return -1;
    }

  dev->addr = addr;
  dev->calib_loaded = 0;
  return 0;
}

void dps310_deinit(struct dps310_device *dev)
{
  if (dev != NULL && dev->fd >= 0)
    {
      PrivClose(dev->fd);
      dev->fd = -1;
    }
}

int dps310_probe(struct dps310_device *dev)
{
  uint8_t id;

  if (dev == NULL || dev->fd < 0) return -1;

  if (dps310_read_reg(dev, DPS310_REG_PRODUCT_ID, &id, 1) < 0) return -1;
  if (id != DPS310_PRODUCT_ID) return -1;
  return 0;
}

int dps310_reset(struct dps310_device *dev)
{
  if (dps310_write_reg(dev, DPS310_REG_RESET, 0x89) < 0) return -1;

  PrivTaskDelay(40);

  return dps310_wait_ready(dev, DPS310_MEAS_COEF_RDY, 200);
}

int dps310_read_calibration(struct dps310_device *dev)
{
  uint8_t buf[18];

  if (dev == NULL) return -1;

  if (dps310_read_reg(dev, DPS310_REG_COEF, buf, 18) < 0) return -1;

  dev->calib.c0  = dps310_twos_complement(
                     ((uint32_t)buf[0] << 4) | (buf[1] >> 4), 12);
  dev->calib.c1  = dps310_twos_complement(
                     (((uint32_t)buf[1] & 0x0F) << 8) | buf[2], 12);
  dev->calib.c00 = dps310_twos_complement(
                     ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | (buf[5] >> 4), 20);
  dev->calib.c10 = dps310_twos_complement(
                     (((uint32_t)buf[5] & 0x0F) << 16) | ((uint32_t)buf[6] << 8) | buf[7], 20);
  dev->calib.c01 = dps310_twos_complement(((uint32_t)buf[8] << 8) | buf[9], 16);
  dev->calib.c11 = dps310_twos_complement(((uint32_t)buf[10] << 8) | buf[11], 16);
  dev->calib.c20 = dps310_twos_complement(((uint32_t)buf[12] << 8) | buf[13], 16);
  dev->calib.c21 = dps310_twos_complement(((uint32_t)buf[14] << 8) | buf[15], 16);
  dev->calib.c30 = dps310_twos_complement(((uint32_t)buf[16] << 8) | buf[17], 16);

  dev->calib_loaded = 1;
  return 0;
}

int dps310_read_temperature(struct dps310_device *dev,
                            int32_t *temp_mcelsius)
{
  uint8_t buf[3];
  int32_t raw;
  float temp;

  if (dev == NULL || temp_mcelsius == NULL) return -1;
  if (!dev->calib_loaded) return -1;

  if (dps310_write_reg(dev, DPS310_REG_TMP_CFG, 0x80) < 0) return -1;
  if (dps310_write_reg(dev, DPS310_REG_MEAS_CFG, DPS310_MEAS_TMP_SINGLE) < 0) return -1;
  if (dps310_wait_ready(dev, DPS310_MEAS_TMP_RDY, 100) < 0) return -1;
  if (dps310_read_reg(dev, DPS310_REG_TMP_B2, buf, 3) < 0) return -1;

  raw = dps310_twos_complement(
          ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2], 24);

  temp = (float)dev->calib.c0 * 0.5f +
         (float)dev->calib.c1 * (float)raw / (float)DPS310_SCALE_FACTOR_1;

  *temp_mcelsius = (int32_t)(temp * 1000.0f);
  return 0;
}

int dps310_read_pressure(struct dps310_device *dev,
                         int32_t *pressure_pa_x100)
{
  uint8_t buf[3];
  int32_t raw_prs, raw_tmp;
  float traw_sc, praw_sc, pressure;

  if (dev == NULL || pressure_pa_x100 == NULL) return -1;
  if (!dev->calib_loaded) return -1;

  /* Temperature for compensation */
  if (dps310_write_reg(dev, DPS310_REG_TMP_CFG, 0x80) < 0) return -1;
  if (dps310_write_reg(dev, DPS310_REG_MEAS_CFG, DPS310_MEAS_TMP_SINGLE) < 0) return -1;
  if (dps310_wait_ready(dev, DPS310_MEAS_TMP_RDY, 100) < 0) return -1;
  if (dps310_read_reg(dev, DPS310_REG_TMP_B2, buf, 3) < 0) return -1;

  raw_tmp = dps310_twos_complement(
              ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2], 24);

  /* Pressure */
  if (dps310_write_reg(dev, DPS310_REG_PRS_CFG, 0x00) < 0) return -1;
  if (dps310_write_reg(dev, DPS310_REG_MEAS_CFG, DPS310_MEAS_PRS_SINGLE) < 0) return -1;
  if (dps310_wait_ready(dev, DPS310_MEAS_PRS_RDY, 100) < 0) return -1;
  if (dps310_read_reg(dev, DPS310_REG_PRS_B2, buf, 3) < 0) return -1;

  raw_prs = dps310_twos_complement(
              ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2], 24);

  traw_sc = (float)raw_tmp / (float)DPS310_SCALE_FACTOR_1;
  praw_sc = (float)raw_prs / (float)DPS310_SCALE_FACTOR_1;

  pressure = (float)dev->calib.c00 +
             praw_sc * ((float)dev->calib.c10 +
                        praw_sc * ((float)dev->calib.c20 + praw_sc * (float)dev->calib.c30)) +
             traw_sc * (float)dev->calib.c01 +
             traw_sc * praw_sc * ((float)dev->calib.c11 + praw_sc * (float)dev->calib.c21);

  *pressure_pa_x100 = (int32_t)(pressure * 100.0f);
  return 0;
}
