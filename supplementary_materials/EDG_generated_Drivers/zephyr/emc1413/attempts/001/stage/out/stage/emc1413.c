#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <stdint.h>
#include <errno.h>
#include "emc1413.h"

#include <zephyr/sys/byteorder.h>
#define EMC1413_REG_MANUFACTURER_ID 0xFE
#define EMC1413_REG_PRODUCT_ID 0xFD
#define EMC1413_REG_CONFIG 0x03
#define EMC1413_REG_CONV_RATE 0x04
#define EMC1413_REG_INT_HIGH 0x00
#define EMC1413_REG_INT_LOW 0x29
#define EMC1413_REG_EXT1_HIGH 0x01
#define EMC1413_REG_EXT1_LOW 0x10
#define EMC1413_REG_EXT2_HIGH 0x23
#define EMC1413_REG_EXT2_LOW 0x24

static int emc1413_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = {reg, val};
	struct i2c_dt_spec spec = {.bus = dev, .addr = EMC1413_I2C_ADDR};
	return i2c_write_dt(&spec, buf, 2);
}

static int emc1413_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	struct i2c_dt_spec spec = {.bus = dev, .addr = EMC1413_I2C_ADDR};
	int ret = i2c_write_dt(&spec, &reg, 1);
	if (ret < 0) return ret;
	return i2c_read_dt(&spec, val, 1);
}

static int emc1413_read_temperature(const struct device *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp_milli)
{
	uint8_t high, low;
	int ret;
	ret = emc1413_read_reg(dev, reg_high, &high);
	if (ret < 0) return ret;
	ret = emc1413_read_reg(dev, reg_low, &low);
	if (ret < 0) return ret;
	*temp_milli = ((int32_t)high * 1000) + ((((low >> 5) & 0x07) * 125));
	return 0;
}

int emc1413_init(const struct device *dev)
{
	uint8_t val;
	int ret;
	k_msleep(15);
	ret = emc1413_read_reg(dev, EMC1413_REG_MANUFACTURER_ID, &val);
	if (ret < 0 || val != 0x5D) return -ENODEV;
	ret = emc1413_read_reg(dev, EMC1413_REG_PRODUCT_ID, &val);
	if (ret < 0 || val != 0x21) return -ENODEV;
	ret = emc1413_write_reg(dev, EMC1413_REG_CONFIG, 0x00);
	if (ret < 0) return ret;
	ret = emc1413_write_reg(dev, EMC1413_REG_CONV_RATE, 0x06);
	if (ret < 0) return ret;
	return 0;
}

int emc1413_read_internal_temperature(const struct device *dev, int32_t *temp_local_val)
{
	return emc1413_read_temperature(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp_local_val);
}

int emc1413_read_external_diode_1_temperature(const struct device *dev, int32_t *temp_ext1_val)
{
	return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp_ext1_val);
}

int emc1413_read_external_diode_2_temperature(const struct device *dev, int32_t *temp_ext2_val)
{
	return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp_ext2_val);
}
