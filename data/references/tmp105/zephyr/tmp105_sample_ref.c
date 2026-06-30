/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include "tmp105_ref.h"

#define I2C_NODE DT_NODELABEL(i2c1)

int main(void)
{
  const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
  struct tmp105_device dev;
  int32_t temp_mc;
  int ret;

  if (!device_is_ready(i2c_dev))
    {
      printk("ERROR: I2C bus not ready\n");
      return -1;
    }

  ret = tmp105_init(&dev, i2c_dev, TMP105_ADDR_DEFAULT);
  if (ret < 0)
    {
      printk("ERROR: tmp105_init failed: %d\n", ret);
      return ret;
    }

  ret = tmp105_probe(&dev);
  if (ret < 0)
    {
      printk("ERROR: TMP105 not found: %d\n", ret);
      return ret;
    }

  printk("[TMP105] addr=0x%02X probe OK\n", TMP105_ADDR_DEFAULT);

  tmp105_set_resolution(&dev, TMP105_RES_12BIT);

  for (int i = 0; i < 5; i++)
    {
      ret = tmp105_read_temperature(&dev, &temp_mc);
      if (ret < 0)
        {
          printk("ERROR: read failed: %d\n", ret);
          return ret;
        }

      printk("[TMP105] sample=%d temp=%d.%03d C\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)(temp_mc >= 0 ? temp_mc % 1000 : (-temp_mc) % 1000));

      k_msleep(1000);
    }

  return 0;
}
