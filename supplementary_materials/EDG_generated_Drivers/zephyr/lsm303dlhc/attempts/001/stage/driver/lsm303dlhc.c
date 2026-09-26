#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <errno.h>
#include "lsm303dlhc.h"

#include <zephyr/sys/byteorder.h>
/* Helper: write a single register via i2c_write_dt */
static int write_reg(const struct device *dev, uint16_t addr, uint8_t reg, uint8_t val)
{
	struct i2c_dt_spec spec = {
		.bus = dev,
		.addr = addr
	};
	uint8_t buf[2] = {reg, val};
	return i2c_write_dt(&spec, buf, 2);
}

/* Helper: write a register address then read multiple bytes via i2c_transfer */
static int write_then_read(const struct device *dev, uint16_t addr, uint8_t reg, uint8_t *buf, uint32_t len)
{
	struct i2c_msg msgs[2];

	msgs[0].buf = &reg;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = buf;
	msgs[1].len = len;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msgs, 2, addr);
}

int lsm303dlhc_init(const struct device *dev)
{
	int ret;

	/* Accelerometer init */
	ret = write_reg(dev, LSM303DLHC_ACCEL_ADDR, LSM303DLHC_CTRL_REG1_A, 0x57);
	if (ret < 0) return -EIO;

	ret = write_reg(dev, LSM303DLHC_ACCEL_ADDR, LSM303DLHC_CTRL_REG4_A, 0x08);
	if (ret < 0) return -EIO;

	k_msleep(70);

	/* Magnetometer init */
	ret = write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_CRA_REG_M, 0x0C);
	if (ret < 0) return -EIO;

	ret = write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_CRB_REG_M, 0x20);
	if (ret < 0) return -EIO;

	ret = write_reg(dev, LSM303DLHC_MAG_ADDR, LSM303DLHC_MR_REG_M, 0x00);
	if (ret < 0) return -EIO;

	k_msleep(1);

	return 0;
}

int lsm303dlhc_read_accel(const struct device *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
	uint8_t buf[6];
	int ret;

	/* Write address 0xA8 (OUT_X_L_A | 0x80 for auto-increment) then read 6 bytes */
	ret = write_then_read(dev, LSM303DLHC_ACCEL_ADDR, 0xA8, buf, 6);
	if (ret < 0) return -EIO;

	/* Big-endian: buf[0]=X_L, buf[1]=X_H, buf[2]=Y_L, buf[3]=Y_H, buf[4]=Z_L, buf[5]=Z_H */
	*ax = (int16_t)((uint16_t)(buf[1] << 8) | buf[0]);
	*ay = (int16_t)((uint16_t)(buf[3] << 8) | buf[2]);
	*az = (int16_t)((uint16_t)(buf[5] << 8) | buf[4]);

	return 0;
}

int lsm303dlhc_read_mag(const struct device *dev, int16_t *mx, int16_t *my, int16_t *mz)
{
	uint8_t buf[6];
	int ret;

	/* Write address 0x03 (OUT_X_H_M) then read 6 bytes: X_H, X_L, Z_H, Z_L, Y_H, Y_L */
	ret = write_then_read(dev, LSM303DLHC_MAG_ADDR, 0x03, buf, 6);
	if (ret < 0) return -EIO;

	*mx = (int16_t)((uint16_t)(buf[0] << 8) | buf[1]);
	*mz = (int16_t)((uint16_t)(buf[2] << 8) | buf[3]);
	*my = (int16_t)((uint16_t)(buf[4] << 8) | buf[5]);

	/* Read temperature */
	ret = write_then_read(dev, LSM303DLHC_MAG_ADDR, 0x31, buf, 2);
	if (ret < 0) return -EIO;

	/* Temperature is stored in a separate call; we ignore it here */
	return 0;
}

int lsm303dlhc_read_temp(const struct device *dev, int32_t *t)
{
	uint8_t buf[2];
	int ret;

	/* Write address 0x31 (TEMP_OUT_H_M) then read 2 bytes */
	ret = write_then_read(dev, LSM303DLHC_MAG_ADDR, 0x31, buf, 2);
	if (ret < 0) return -EIO;

	int16_t raw = (int16_t)((uint16_t)(buf[0] << 8) | buf[1]);
	*t = (int32_t)raw * 125;

	return 0;
}
