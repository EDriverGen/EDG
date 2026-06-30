#ifndef LM75A_CMSIS_RTX_REF_H
#define LM75A_CMSIS_RTX_REF_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define LM75A_ADDR_MIN      0x48
#define LM75A_ADDR_MAX      0x4F
#define LM75A_DEFAULT_ADDR  LM75A_ADDR_MIN

#define LM75A_REG_TEMP      0x00
#define LM75A_REG_CONF      0x01
#define LM75A_REG_THYST     0x02
#define LM75A_REG_TOS       0x03

#define LM75A_TEMP_STEP_MC  125

struct lm75a_device {
    I2C_HandleTypeDef *bus;
    uint8_t addr;
};

int lm75a_init(struct lm75a_device *dev, I2C_HandleTypeDef *bus, uint8_t addr);
int lm75a_probe(struct lm75a_device *dev);
int lm75a_read_raw(struct lm75a_device *dev, int16_t *raw);
int32_t lm75a_raw_to_mcelsius(int16_t raw);
int lm75a_read_temp_mcelsius(struct lm75a_device *dev, int32_t *temp_mc);

#endif
