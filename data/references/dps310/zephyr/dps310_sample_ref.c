/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Digital Pressure Sensor Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include "dps310_ref.h"

#define I2C_NODE DT_NODELABEL(i2c1)

int main(void)
{
  const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
  struct dps310_device dev;
  int32_t temp_mc, pressure;
  int ret;

  if (!device_is_ready(i2c_dev))
    {
      printk("ERROR: I2C bus not ready\n");
      return -1;
    }

  ret = dps310_init(&dev, i2c_dev, DPS310_ADDR_DEFAULT);
  if (ret < 0) { printk("ERROR: init %d\n", ret); return ret; }

  ret = dps310_probe(&dev);
  if (ret < 0) { printk("ERROR: probe %d\n", ret); return ret; }

  printk("[DPS310] addr=0x%02X probe OK\n", DPS310_ADDR_DEFAULT);

  dps310_reset(&dev);

  ret = dps310_read_calibration(&dev);
  if (ret < 0) { printk("ERROR: calib %d\n", ret); return ret; }

  for (int i = 0; i < 5; i++)
    {
      ret = dps310_read_temperature(&dev, &temp_mc);
      if (ret < 0) { printk("ERROR: temp %d\n", ret); return ret; }

      ret = dps310_read_pressure(&dev, &pressure);
      if (ret < 0) { printk("ERROR: prs %d\n", ret); return ret; }

      printk("[DPS310] sample=%d temp=%d.%02d C pressure=%d.%02d Pa\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)((temp_mc >= 0 ? temp_mc : -temp_mc) % 1000 / 10),
             (int)(pressure / 100),
             (int)((pressure >= 0 ? pressure : -pressure) % 100));

      k_msleep(1000);
    }

  return 0;
}
