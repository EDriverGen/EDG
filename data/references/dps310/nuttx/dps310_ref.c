/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Digital Pressure Sensor Driver for NuttX
 */
#include "dps310_ref.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* ---- Internal helpers ---- */

static int dps310_read_reg(FAR struct dps310_device *dev,
                           uint8_t reg, FAR uint8_t *buf, int len)
{
  return i2c_writeread(dev->i2c, &dev->config, &reg, 1, buf, len);
}

static int dps310_write_reg(FAR struct dps310_device *dev,
                            uint8_t reg, uint8_t value)
{
  uint8_t frame[2];

  frame[0] = reg;
  frame[1] = value;
  return i2c_write(dev->i2c, &dev->config, frame, 2);
}

/*
 * Sign-extend a value from 'bits' width to int32_t.
 */
static int32_t dps310_twos_complement(uint32_t val, uint8_t bits)
{
  if (val & ((uint32_t)1 << (bits - 1)))
    {
      return (int32_t)(val | (~(uint32_t)0 << bits));
    }

  return (int32_t)val;
}

static int dps310_wait_ready(FAR struct dps310_device *dev,
                             uint8_t mask, int timeout_ms)
{
  uint8_t meas_cfg;
  int elapsed = 0;
  int ret;

  while (elapsed < timeout_ms)
    {
      ret = dps310_read_reg(dev, DPS310_REG_MEAS_CFG, &meas_cfg, 1);
      if (ret < 0) return ret;

      if (meas_cfg & mask)
        {
          return 0;
        }

      usleep(10000);
      elapsed += 10;
    }

  return -ETIMEDOUT;
}

/* ---- Public API ---- */

int dps310_init(FAR struct dps310_device *dev,
                FAR struct i2c_master_s *i2c,
                uint8_t addr)
{
  if (dev == NULL || i2c == NULL)
    {
      return -EINVAL;
    }

  memset(dev, 0, sizeof(*dev));
  dev->i2c = i2c;
  dev->config.frequency = DPS310_I2C_FREQ;
  dev->config.address   = addr;
  dev->config.addrlen   = 7;
  dev->calib_loaded = false;

  return 0;
}

int dps310_probe(FAR struct dps310_device *dev)
{
  uint8_t id;
  int ret;

  if (dev == NULL || dev->i2c == NULL)
    {
      return -EINVAL;
    }

  ret = dps310_read_reg(dev, DPS310_REG_PRODUCT_ID, &id, 1);
  if (ret < 0) return ret;

  if (id != DPS310_PRODUCT_ID)
    {
      return -ENODEV;
    }

  return 0;
}

int dps310_reset(FAR struct dps310_device *dev)
{
  int ret;

  ret = dps310_write_reg(dev, DPS310_REG_RESET, 0x89); /* soft reset */
  if (ret < 0) return ret;

  usleep(40000); /* 40 ms for reset */

  /* Wait for coefficients ready */
  return dps310_wait_ready(dev, DPS310_MEAS_COEF_RDY, 200);
}

int dps310_read_calibration(FAR struct dps310_device *dev)
{
  uint8_t buf[18];
  int ret;

  if (dev == NULL)
    {
      return -EINVAL;
    }

  ret = dps310_read_reg(dev, DPS310_REG_COEF, buf, 18);
  if (ret < 0) return ret;

  dev->calib.c0  = dps310_twos_complement(
                     ((uint32_t)buf[0] << 4) | (buf[1] >> 4), 12);
  dev->calib.c1  = dps310_twos_complement(
                     (((uint32_t)buf[1] & 0x0F) << 8) | buf[2], 12);
  dev->calib.c00 = dps310_twos_complement(
                     ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | (buf[5] >> 4), 20);
  dev->calib.c10 = dps310_twos_complement(
                     (((uint32_t)buf[5] & 0x0F) << 16) | ((uint32_t)buf[6] << 8) | buf[7], 20);
  dev->calib.c01 = dps310_twos_complement(
                     ((uint32_t)buf[8] << 8) | buf[9], 16);
  dev->calib.c11 = dps310_twos_complement(
                     ((uint32_t)buf[10] << 8) | buf[11], 16);
  dev->calib.c20 = dps310_twos_complement(
                     ((uint32_t)buf[12] << 8) | buf[13], 16);
  dev->calib.c21 = dps310_twos_complement(
                     ((uint32_t)buf[14] << 8) | buf[15], 16);
  dev->calib.c30 = dps310_twos_complement(
                     ((uint32_t)buf[16] << 8) | buf[17], 16);

  dev->calib_loaded = true;
  return 0;
}

int dps310_read_temperature(FAR struct dps310_device *dev,
                            FAR int32_t *temp_mcelsius)
{
  uint8_t buf[3];
  int32_t raw_sc;
  int32_t raw;
  float temp;
  int ret;

  if (dev == NULL || temp_mcelsius == NULL)
    {
      return -EINVAL;
    }

  if (!dev->calib_loaded)
    {
      return -EIO;
    }

  /* Configure: 1x oversampling, external sensor */
  ret = dps310_write_reg(dev, DPS310_REG_TMP_CFG, 0x80);
  if (ret < 0) return ret;

  /* Trigger single temperature measurement */
  ret = dps310_write_reg(dev, DPS310_REG_MEAS_CFG, DPS310_MEAS_TMP_SINGLE);
  if (ret < 0) return ret;

  ret = dps310_wait_ready(dev, DPS310_MEAS_TMP_RDY, 100);
  if (ret < 0) return ret;

  ret = dps310_read_reg(dev, DPS310_REG_TMP_B2, buf, 3);
  if (ret < 0) return ret;

  raw = dps310_twos_complement(
          ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2], 24);

  raw_sc = raw; /* scale factor = 1 for 1x oversampling */

  temp = (float)dev->calib.c0 * 0.5f +
         (float)dev->calib.c1 * (float)raw_sc / (float)DPS310_SCALE_FACTOR_1;

  *temp_mcelsius = (int32_t)(temp * 1000.0f);
  return 0;
}

int dps310_read_pressure(FAR struct dps310_device *dev,
                         FAR int32_t *pressure_pa_x100)
{
  uint8_t buf[3];
  int32_t raw_prs, raw_tmp;
  float traw_sc, praw_sc;
  float pressure;
  int ret;

  if (dev == NULL || pressure_pa_x100 == NULL)
    {
      return -EINVAL;
    }

  if (!dev->calib_loaded)
    {
      return -EIO;
    }

  /* Read temperature first (needed for compensation) */
  ret = dps310_write_reg(dev, DPS310_REG_TMP_CFG, 0x80);
  if (ret < 0) return ret;

  ret = dps310_write_reg(dev, DPS310_REG_MEAS_CFG, DPS310_MEAS_TMP_SINGLE);
  if (ret < 0) return ret;

  ret = dps310_wait_ready(dev, DPS310_MEAS_TMP_RDY, 100);
  if (ret < 0) return ret;

  ret = dps310_read_reg(dev, DPS310_REG_TMP_B2, buf, 3);
  if (ret < 0) return ret;

  raw_tmp = dps310_twos_complement(
              ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2], 24);

  /* Read pressure */
  ret = dps310_write_reg(dev, DPS310_REG_PRS_CFG, 0x00); /* 1x oversampling */
  if (ret < 0) return ret;

  ret = dps310_write_reg(dev, DPS310_REG_MEAS_CFG, DPS310_MEAS_PRS_SINGLE);
  if (ret < 0) return ret;

  ret = dps310_wait_ready(dev, DPS310_MEAS_PRS_RDY, 100);
  if (ret < 0) return ret;

  ret = dps310_read_reg(dev, DPS310_REG_PRS_B2, buf, 3);
  if (ret < 0) return ret;

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
