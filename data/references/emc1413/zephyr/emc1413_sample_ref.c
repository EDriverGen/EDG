/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 3-Channel Temperature Sensor Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include "emc1413_ref.h"

#define I2C_NODE DT_NODELABEL(i2c1)

int main(void)
{
  const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
  struct emc1413_device dev;
  int32_t internal_mc, ext1_mc, ext2_mc;
  int ret;

  if (!device_is_ready(i2c_dev))
    {
      printk("ERROR: I2C bus not ready\n");
      return -1;
    }

  ret = emc1413_init(&dev, i2c_dev, EMC1413_ADDR_DEFAULT);
  if (ret < 0) { printk("ERROR: init %d\n", ret); return ret; }

  ret = emc1413_probe(&dev);
  if (ret < 0) { printk("ERROR: probe %d\n", ret); return ret; }

  printk("[EMC1413] addr=0x%02X probe OK\n", EMC1413_ADDR_DEFAULT);

  for (int i = 0; i < 5; i++)
    {
      ret = emc1413_read_internal_temp(&dev, &internal_mc);
      if (ret < 0) { printk("ERROR: internal %d\n", ret); return ret; }

      ret = emc1413_read_ext1_temp(&dev, &ext1_mc);
      if (ret < 0) { printk("ERROR: ext1 %d\n", ret); return ret; }

      ret = emc1413_read_ext2_temp(&dev, &ext2_mc);
      if (ret < 0) { printk("ERROR: ext2 %d\n", ret); return ret; }

      printk("[EMC1413] sample=%d internal=%d.%03d C ext1=%d.%03d C ext2=%d.%03d C\n",
             i + 1,
             (int)(internal_mc / 1000),
             (int)(internal_mc >= 0 ? internal_mc % 1000 : (-internal_mc) % 1000),
             (int)(ext1_mc / 1000),
             (int)(ext1_mc >= 0 ? ext1_mc % 1000 : (-ext1_mc) % 1000),
             (int)(ext2_mc / 1000),
             (int)(ext2_mc >= 0 ? ext2_mc % 1000 : (-ext2_mc) % 1000));

      k_msleep(1000);
    }

  return 0;
}
