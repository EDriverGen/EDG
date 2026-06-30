/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BH1750 Light Sensor Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include "bh1750_ref.h"

#define I2C_NODE DT_NODELABEL(i2c1)

int main(void)
{
  const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
  struct bh1750_device dev;
  uint32_t lux_x100;
  int ret;

  if (!device_is_ready(i2c_dev))
    {
      printk("ERROR: I2C bus not ready\n");
      return -1;
    }

  ret = bh1750_init(&dev, i2c_dev, BH1750_DEFAULT_ADDR);
  if (ret < 0)
    {
      printk("ERROR: bh1750_init failed: %d\n", ret);
      return ret;
    }

  ret = bh1750_probe(&dev);
  if (ret < 0)
    {
      printk("ERROR: BH1750 not found at 0x%02X: %d\n",
             BH1750_DEFAULT_ADDR, ret);
      return ret;
    }

  printk("[BH1750] addr=0x%02X probe OK\n", BH1750_DEFAULT_ADDR);

  for (int i = 0; i < 5; i++)
    {
      ret = bh1750_read_lux_x100(&dev, &lux_x100);
      if (ret < 0)
        {
          printk("ERROR: read failed: %d\n", ret);
          return ret;
        }

      printk("[BH1750] sample=%d lux=%u.%02u\n",
             i + 1,
             (unsigned)(lux_x100 / 100),
             (unsigned)(lux_x100 % 100));

      k_msleep(1000);
    }

  return 0;
}
