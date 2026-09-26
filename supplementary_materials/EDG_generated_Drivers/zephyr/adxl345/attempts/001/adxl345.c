#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define ADXL345_REG_POWER_CTL 0x2D
#define ADXL345_REG_DATAX0    0x32
#define ADXL345_READ_CMD(reg) (0x80 | (reg))
#define ADXL345_MB_CMD(reg)   (0xC0 | (reg))

static const struct spi_dt_spec adxl345_spi = {
	.bus = DEVICE_DT_GET(DT_NODELABEL(spi1)),
	.config = {
		.frequency = 5000000,
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
		.slave = 0,
		.cs = {
			.gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(spi1), cs_gpios),
			.delay = 0,
		},
	},
};

static int spi_write_reg(const struct spi_dt_spec *spec, uint8_t reg, uint8_t val)
{
	uint8_t tx_buf[2] = {reg, val};
	const struct spi_buf tx_bufs = {.buf = tx_buf, .len = 2};
	const struct spi_buf_set tx = {.buffers = &tx_bufs, .count = 1};
	return spi_transceive_dt(spec, &tx, NULL);
}

static int spi_read_regs(const struct spi_dt_spec *spec, uint8_t reg, uint8_t *buf, size_t len)
{
	uint8_t cmd = (len > 1) ? ADXL345_MB_CMD(reg) : ADXL345_READ_CMD(reg);
	uint8_t tx_buf[1] = {cmd};
	const struct spi_buf tx_bufs = {.buf = tx_buf, .len = 1};
	const struct spi_buf rx_bufs = {.buf = buf, .len = len};
	const struct spi_buf_set tx = {.buffers = &tx_bufs, .count = 1};
	const struct spi_buf_set rx = {.buffers = &rx_bufs, .count = 1};
	return spi_transceive_dt(spec, &tx, &rx);
}

int adxl345_init(const struct device *dev)
{
	int ret;
	(void)dev;

	ret = spi_write_reg(&adxl345_spi, ADXL345_REG_POWER_CTL, 0x00);
	if (ret < 0)
		return -EIO;

	ret = spi_write_reg(&adxl345_spi, ADXL345_REG_POWER_CTL, 0x08);
	if (ret < 0)
		return -EIO;

	k_sleep(K_MSEC(12));

	return 0;
}

int adxl345_read_xyz(const struct device *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
	uint8_t buf[6];
	int ret;
	(void)dev;

	ret = spi_read_regs(&adxl345_spi, ADXL345_REG_DATAX0, buf, 6);
	if (ret < 0)
		return -EIO;

	*ax = (int16_t)sys_le16_to_cpu(*(uint16_t *)&buf[0]);
	*ay = (int16_t)sys_le16_to_cpu(*(uint16_t *)&buf[2]);
	*az = (int16_t)sys_le16_to_cpu(*(uint16_t *)&buf[4]);

	return 0;
}
