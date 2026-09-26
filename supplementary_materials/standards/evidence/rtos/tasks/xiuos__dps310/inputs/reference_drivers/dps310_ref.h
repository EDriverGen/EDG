/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Digital Pressure Sensor Driver for XiUOS
 */
#ifndef __DPS310_REF_H
#define __DPS310_REF_H

#include <transform.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{

#endif

#define DPS310_ADDR_DEFAULT          0x77
#define DPS310_DEFAULT_ADDR  DPS310_ADDR_DEFAULT  /* alias */

#define DPS310_REG_PRS_B2            0x00
#define DPS310_REG_TMP_B2            0x03
#define DPS310_REG_PRS_CFG           0x06
#define DPS310_REG_TMP_CFG           0x07
#define DPS310_REG_MEAS_CFG          0x08
#define DPS310_REG_CFG_REG           0x09
#define DPS310_REG_RESET             0x0C
#define DPS310_REG_PRODUCT_ID        0x0D
#define DPS310_REG_COEF              0x10

#define DPS310_MEAS_COEF_RDY         (1U << 7)
#define DPS310_MEAS_TMP_RDY          (1U << 5)
#define DPS310_MEAS_PRS_RDY          (1U << 4)
#define DPS310_MEAS_TMP_SINGLE       0x02
#define DPS310_MEAS_PRS_SINGLE       0x01

#define DPS310_PRODUCT_ID            0x10
#define DPS310_SCALE_FACTOR_1        524288

struct dps310_calib
{
  int32_t c0, c1, c00, c10, c01, c11, c20, c21, c30;
};

struct dps310_device
{
  int fd;
  uint16_t addr;
  struct dps310_calib calib;
  int calib_loaded;
};

int dps310_init(struct dps310_device *dev,
                const char *i2c_dev_path,
                uint16_t addr);
void dps310_deinit(struct dps310_device *dev);
int dps310_probe(struct dps310_device *dev);
int dps310_reset(struct dps310_device *dev);
int dps310_read_calibration(struct dps310_device *dev);
int dps310_read_temperature(struct dps310_device *dev,
                            int32_t *temp_mcelsius);
int dps310_read_pressure(struct dps310_device *dev,
                         int32_t *pressure_pa_x100);

#ifdef __cplusplus
}
#endif

#endif /* __DPS310_REF_H */
