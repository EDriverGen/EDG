#ifndef DPS310_CMSIS_RTX_REF_H
#define DPS310_CMSIS_RTX_REF_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define DPS310_ADDR_LOW 0x76
#define DPS310_ADDR_HIGH 0x77
#define DPS310_DEFAULT_ADDR DPS310_ADDR_HIGH
#define DPS310_REG_PSR_B2 0x00
#define DPS310_REG_TMP_B2 0x03
#define DPS310_REG_MEAS_CFG 0x08
#define DPS310_REG_RESET 0x0C
#define DPS310_REG_PRODUCT_ID 0x0D
#define DPS310_REG_COEF 0x10
#define DPS310_REG_TMP_CFG 0x07
#define DPS310_REG_COEF_SRCE 0x28
#define DPS310_MEAS_CFG_PRS_RDY (1U << 4)
#define DPS310_MEAS_CFG_TMP_RDY (1U << 5)
#define DPS310_MEAS_CFG_SENSOR_RDY (1U << 6)
#define DPS310_MEAS_CFG_COEF_RDY (1U << 7)
#define DPS310_MODE_TMP_SINGLE 0x02
#define DPS310_MODE_PRS_SINGLE 0x01
#define DPS310_RESET_SOFT 0x89
#define DPS310_PRODUCT_ID 0x10
#define DPS310_SCALE_FACTOR_1 524288

struct dps310_calib_coeff { int32_t c0, c1, c00, c10, c01, c11, c20, c21, c30; };
struct dps310_device {
    I2C_HandleTypeDef *bus;
    uint8_t addr;
    struct dps310_calib_coeff coeff;
    int32_t kT;
    int32_t kP;
};

int dps310_init(struct dps310_device *dev, I2C_HandleTypeDef *bus, uint8_t addr);
int dps310_probe(struct dps310_device *dev);
int dps310_reset(struct dps310_device *dev);
int dps310_read_calibration(struct dps310_device *dev);
int dps310_read_temperature(struct dps310_device *dev, int32_t *temp_c100);
int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_pa100);

#endif
