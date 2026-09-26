/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add DPS310 driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_DPS310_H_
#define DRIVERS_INCLUDE_DPS310_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * The DPS310 7-bit I2C address is selected by the SDO pin:
 *   SDO = GND -> 0x76
 *   SDO = V+  -> 0x77
 */
#define DPS310_ADDR_LOW              0x76
#define DPS310_ADDR_HIGH             0x77

#define DPS310_DEFAULT_BUS_NAME      "i2c1"
#define DPS310_DEFAULT_ADDR          DPS310_ADDR_HIGH

/*
 * DPS310 register definitions (datasheet Section 7).
 */
#define DPS310_REG_PSR_B2            0x00  /* Pressure data [23:16] */
#define DPS310_REG_PSR_B1            0x01  /* Pressure data [15:8]  */
#define DPS310_REG_PSR_B0            0x02  /* Pressure data [7:0]   */
#define DPS310_REG_TMP_B2            0x03  /* Temperature data [23:16] */
#define DPS310_REG_TMP_B1            0x04  /* Temperature data [15:8]  */
#define DPS310_REG_TMP_B0            0x05  /* Temperature data [7:0]   */
#define DPS310_REG_PRS_CFG           0x06  /* Pressure measurement configuration */
#define DPS310_REG_TMP_CFG           0x07  /* Temperature measurement configuration */
#define DPS310_REG_MEAS_CFG          0x08  /* Sensor operating mode and status */
#define DPS310_REG_CFG_REG           0x09  /* Interrupt and FIFO configuration */
#define DPS310_REG_INT_STS           0x0A  /* Interrupt status */
#define DPS310_REG_FIFO_STS          0x0B  /* FIFO status */
#define DPS310_REG_RESET             0x0C  /* Software reset */
#define DPS310_REG_PRODUCT_ID        0x0D  /* Product and revision ID */
#define DPS310_REG_COEF              0x10  /* Calibration-coefficient start address (0x10~0x21) */
#define DPS310_REG_COEF_SRCE         0x28  /* Temperature-calibration coefficient source */

/* MEAS_CFG register bits */
#define DPS310_MEAS_CFG_PRS_RDY      (1U << 4)
#define DPS310_MEAS_CFG_TMP_RDY      (1U << 5)
#define DPS310_MEAS_CFG_SENSOR_RDY   (1U << 6)
#define DPS310_MEAS_CFG_COEF_RDY     (1U << 7)

/* Measurement modes (MEAS_CFG[2:0]) */
#define DPS310_MODE_IDLE             0x00
#define DPS310_MODE_PRS_SINGLE       0x01
#define DPS310_MODE_TMP_SINGLE       0x02
#define DPS310_MODE_PRS_CONT         0x05
#define DPS310_MODE_TMP_CONT         0x06
#define DPS310_MODE_PRS_TMP_CONT     0x07

/* Software-reset command */
#define DPS310_RESET_SOFT            0x89

/* Expected product ID (REV_ID | PROD_ID) */
#define DPS310_PRODUCT_ID            0x10

/*
 * Scale factors for the oversampling rates.
 * From datasheet Section 4.9.1.
 */
#define DPS310_SCALE_FACTOR_1        524288
#define DPS310_SCALE_FACTOR_2        1572864
#define DPS310_SCALE_FACTOR_4        3670016
#define DPS310_SCALE_FACTOR_8        7864320
#define DPS310_SCALE_FACTOR_16       253952
#define DPS310_SCALE_FACTOR_32       516096
#define DPS310_SCALE_FACTOR_64       1040384
#define DPS310_SCALE_FACTOR_128      2088960

/*
 * DPS310 calibration-coefficient structure.
 * Read once from the chip's NVRAM after power-up.
 */
struct dps310_calib_coeff
{
    rt_int32_t c0;
    rt_int32_t c1;
    rt_int32_t c00;
    rt_int32_t c10;
    rt_int32_t c01;
    rt_int32_t c11;
    rt_int32_t c20;
    rt_int32_t c21;
    rt_int32_t c30;
};

/*
 * DPS310 driver device object.
 */
struct dps310_device
{
    struct rt_i2c_bus_device *bus;
    const char *bus_name;
    rt_uint8_t addr;
    struct dps310_calib_coeff coeff;
    rt_int32_t kT;  /* Temperature scale factor */
    rt_int32_t kP;  /* Pressure scale factor */
};

/*
 * Initialize a DPS310 device object.
 */
rt_err_t dps310_init(struct dps310_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr);

/*
 * Probe DPS310 by reading its product ID.
 */
rt_err_t dps310_probe(struct dps310_device *dev);

/*
 * Software reset.
 */
rt_err_t dps310_reset(struct dps310_device *dev);

/*
 * Read calibration coefficients; call once after initialization.
 */
rt_err_t dps310_read_calibration(struct dps310_device *dev);

/*
 * Trigger and read a temperature measurement (degrees Celsius * 100).
 */
rt_err_t dps310_read_temperature(struct dps310_device *dev,
                                 rt_int32_t           *temp_c100);

/*
 * Trigger and read a pressure measurement (Pa * 100).
 * A preceding temperature measurement is required for compensation.
 */
rt_err_t dps310_read_pressure(struct dps310_device *dev,
                              rt_int32_t           *pressure_pa100);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_DPS310_H_ */
