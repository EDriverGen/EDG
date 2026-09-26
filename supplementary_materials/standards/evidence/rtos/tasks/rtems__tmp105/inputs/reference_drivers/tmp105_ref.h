#ifndef TMP105_RTEMS_REF_H
#define TMP105_RTEMS_REF_H

#include <stdint.h>
#include <rtems.h>

#define TMP105_ADDR_LOW       0x48
#define TMP105_ADDR_HIGH      0x49
#define TMP105_DEFAULT_ADDR   TMP105_ADDR_LOW

#define TMP105_REG_TEMP       0x00
#define TMP105_REG_CONFIG     0x01
#define TMP105_REG_T_LOW      0x02
#define TMP105_REG_T_HIGH     0x03

#define TMP105_CONF_RES_0     (1U << 5)
#define TMP105_CONF_RES_1     (1U << 6)

struct tmp105_device {
    const char *bus_path;
    uint8_t addr;
};

int tmp105_init(struct tmp105_device *dev, const char *bus_path, uint8_t addr);
int tmp105_probe(struct tmp105_device *dev);
int tmp105_read_temperature(struct tmp105_device *dev, int32_t *temp_mcelsius);
int tmp105_set_resolution(struct tmp105_device *dev, uint8_t bits);
int tmp105_read_config(struct tmp105_device *dev, uint8_t *config);

#endif
