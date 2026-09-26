/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Digital Pressure Sensor Driver for NuttX
 */
#ifndef __DPS310_REF_H
#define __DPS310_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{

#endif

#define DPS310_ADDR_DEFAULT          0x77
#define DPS310_DEFAULT_ADDR  DPS310_ADDR_DEFAULT  /* alias */
#define DPS310_ADDR_ALT              0x76
#define DPS310_I2C_FREQ              100000

/* Register Map */
#define DPS310_REG_PRS_B2            0x00
#define DPS310_REG_PRS_B1            0x01
#define DPS310_REG_PRS_B0            0x02
#define DPS310_REG_TMP_B2            0x03
#define DPS310_REG_TMP_B1            0x04
#define DPS310_REG_TMP_B0            0x05
#define DPS310_REG_PRS_CFG           0x06
#define DPS310_REG_TMP_CFG           0x07
#define DPS310_REG_MEAS_CFG          0x08
#define DPS310_REG_CFG_REG           0x09
#define DPS310_REG_RESET             0x0C
#define DPS310_REG_PRODUCT_ID        0x0D
#define DPS310_REG_COEF              0x10  /* 0x10 ~ 0x21 (18 bytes) */
#define DPS310_REG_COEF_SRC          0x28

/* MEAS_CFG bits */
#define DPS310_MEAS_COEF_RDY         (1U << 7)
#define DPS310_MEAS_SENSOR_RDY       (1U << 6)
#define DPS310_MEAS_TMP_RDY          (1U << 5)
#define DPS310_MEAS_PRS_RDY          (1U << 4)
#define DPS310_MEAS_TMP_SINGLE       0x02
#define DPS310_MEAS_PRS_SINGLE       0x01

/* Product ID expected value */
#define DPS310_PRODUCT_ID            0x10

/* Oversampling scale factors */
#define DPS310_SCALE_FACTOR_1        524288
#define DPS310_SCALE_FACTOR_DEFAULT  DPS310_SCALE_FACTOR_1

struct dps310_calib
{
  int32_t c0;
  int32_t c1;
  int32_t c00;
  int32_t c10;
  int32_t c01;
  int32_t c11;
  int32_t c20;
  int32_t c21;
  int32_t c30;
};

struct dps310_device
{
  FAR struct i2c_master_s *i2c;
  struct i2c_config_s config;
  struct dps310_calib calib;
  bool calib_loaded;
};

int dps310_init(FAR struct dps310_device *dev,
                FAR struct i2c_master_s *i2c,
                uint8_t addr);
int dps310_probe(FAR struct dps310_device *dev);
int dps310_reset(FAR struct dps310_device *dev);
int dps310_read_calibration(FAR struct dps310_device *dev);

int dps310_read_temperature(FAR struct dps310_device *dev,
                            FAR int32_t *temp_mcelsius);
int dps310_read_pressure(FAR struct dps310_device *dev,
                         FAR int32_t *pressure_pa_x100);

#ifdef __cplusplus
}
#endif

#endif /* __DPS310_REF_H */
