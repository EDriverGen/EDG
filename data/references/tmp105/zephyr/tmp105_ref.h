/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Driver for Zephyr
 */
#ifndef __TMP105_REF_H
#define __TMP105_REF_H

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define TMP105_ADDR_DEFAULT          0x48

#define TMP105_REG_TEMP              0x00
#define TMP105_REG_CONF              0x01
#define TMP105_REG_TLOW              0x02
#define TMP105_REG_THIGH             0x03

#define TMP105_CONF_SD               (1U << 0)
#define TMP105_CONF_TM               (1U << 1)
#define TMP105_CONF_POL              (1U << 2)
#define TMP105_CONF_R0               (1U << 5)
#define TMP105_CONF_R1               (1U << 6)
#define TMP105_CONF_OS               (1U << 7)

#define TMP105_RES_9BIT              0
#define TMP105_RES_10BIT             1
#define TMP105_RES_11BIT             2
#define TMP105_RES_12BIT             3

struct tmp105_device
{
  const struct device *bus;
  uint16_t addr;
  uint8_t resolution;
};

int tmp105_init(struct tmp105_device *dev,
                const struct device *bus,
                uint16_t addr);
int tmp105_probe(struct tmp105_device *dev);
int tmp105_set_resolution(struct tmp105_device *dev, uint8_t res);
int tmp105_read_temperature(struct tmp105_device *dev,
                            int32_t *temp_mcelsius);
int tmp105_read_config(struct tmp105_device *dev, uint8_t *config);
int tmp105_write_config(struct tmp105_device *dev, uint8_t config);

#ifdef __cplusplus
}
#endif

#endif /* __TMP105_REF_H */
