#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define MPU6050_I2C_ADDR 0x68

#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_WHO_AM_I   0x75
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

int mpu6050_init(const struct device *dev)
{
	uint8_t buf[2];
	int ret;

	/* Wake up device: clear SLEEP bit */
	buf[0] = MPU6050_REG_PWR_MGMT_1;
	buf[1] = 0x00;
	ret = i2c_write_dt(&(const struct i2c_dt_spec){.bus = dev, .addr = MPU6050_I2C_ADDR}, buf, 2);
	if (ret < 0) {
		return ret;
	}

	/* Wait 100 ms for stabilization */
	k_msleep(100);

	/* Probe: read WHO_AM_I */
	buf[0] = MPU6050_REG_WHO_AM_I;
	ret = i2c_write_dt(&(const struct i2c_dt_spec){.bus = dev, .addr = MPU6050_I2C_ADDR}, buf, 1);
	if (ret < 0) {
		return ret;
	}
	uint8_t whoami;
	ret = i2c_read_dt(&(const struct i2c_dt_spec){.bus = dev, .addr = MPU6050_I2C_ADDR}, &whoami, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int mpu6050_read_all(const struct device *dev, int16_t *ax, int16_t *ay, int16_t *az, int32_t *temp, int16_t *gx, int16_t *gy, int16_t *gz)
{
	uint8_t reg = MPU6050_REG_ACCEL_XOUT_H;
	uint8_t buf[14];
	int ret;

	/* Write register address */
	ret = i2c_write_dt(&(const struct i2c_dt_spec){.bus = dev, .addr = MPU6050_I2C_ADDR}, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	/* Read 14 bytes */
	ret = i2c_read_dt(&(const struct i2c_dt_spec){.bus = dev, .addr = MPU6050_I2C_ADDR}, buf, 14);
	if (ret < 0) {
		return ret;
	}

	/* Parse big-endian 16-bit values */
	int16_t raw_ax = (int16_t)((buf[0] << 8) | buf[1]);
	int16_t raw_ay = (int16_t)((buf[2] << 8) | buf[3]);
	int16_t raw_az = (int16_t)((buf[4] << 8) | buf[5]);
	int16_t raw_temp = (int16_t)((buf[6] << 8) | buf[7]);
	int16_t raw_gx = (int16_t)((buf[8] << 8) | buf[9]);
	int16_t raw_gy = (int16_t)((buf[10] << 8) | buf[11]);
	int16_t raw_gz = (int16_t)((buf[12] << 8) | buf[13]);

	*ax = raw_ax;
	*ay = raw_ay;
	*az = raw_az;
	*gx = raw_gx;
	*gy = raw_gy;
	*gz = raw_gz;

	/* Temperature conversion: ((raw * 1000) / 340) + 36530 */
	int32_t temp_val = ((int32_t)raw_temp * 1000) / 340 + 36530;
	*temp = temp_val;

	return 0;
}
