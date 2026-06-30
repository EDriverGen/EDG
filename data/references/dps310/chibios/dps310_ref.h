/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Pressure/Temperature Sensor Driver for ChibiOS
 */
#ifndef __DPS310_REF_H
#define __DPS310_REF_H

#include "hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DPS310_ADDR_LOW               0x76
#define DPS310_ADDR_HIGH              0x77
#define DPS310_DEFAULT_ADDR           0x77

#define DPS310_REG_PSR_B2             0x00
#define DPS310_REG_TMP_B2             0x03
#define DPS310_REG_PRS_CFG            0x06
#define DPS310_REG_TMP_CFG            0x07
#define DPS310_REG_MEAS_CFG           0x08
#define DPS310_REG_CFG_REG            0x09
#define DPS310_REG_RESET              0x0C
#define DPS310_REG_PROD_ID            0x0D
#define DPS310_REG_COEF_START         0x10
#define DPS310_PROD_ID_EXPECTED       0x10

struct dps310_device
{
    I2CDriver *bus;     /* ChibiOS I2C driver pointer */
    uint16_t addr;
    int32_t c0, c1, c00, c10, c01, c11, c20, c21, c30;
    uint8_t coef_ready;
};

int dps310_init(struct dps310_device *dev, I2CDriver *bus, uint16_t addr);
int dps310_probe(struct dps310_device *dev);
int dps310_read_calibration(struct dps310_device *dev);
int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_pa);
int dps310_read_temperature(struct dps310_device *dev, int32_t *temp_mcelsius);

#ifdef __cplusplus
}
#endif

#endif /* __DPS310_REF_H */
