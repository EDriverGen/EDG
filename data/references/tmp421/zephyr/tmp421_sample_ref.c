/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include "tmp421_ref.h"

#define I2C_NODE DT_NODELABEL(i2c1)

int main(void)
{
  const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
  struct tmp421_device dev;
  int32_t local_mc, remote_mc;
  int ret;

  if (!device_is_ready(i2c_dev))
    {
      printk("ERROR: I2C bus not ready\n");
      return -1;
    }

  ret = tmp421_init(&dev, i2c_dev, TMP421_ADDR_DEFAULT);
  if (ret < 0) { printk("ERROR: init %d\n", ret); return ret; }

  ret = tmp421_probe(&dev);
  if (ret < 0) { printk("ERROR: probe %d\n", ret); return ret; }

  printk("[TMP421] addr=0x%02X probe OK\n", TMP421_ADDR_DEFAULT);

  for (int i = 0; i < 5; i++)
    {
      ret = tmp421_read_local_temp(&dev, &local_mc);
      if (ret < 0) { printk("ERROR: local %d\n", ret); return ret; }

      ret = tmp421_read_remote_temp(&dev, &remote_mc);
      if (ret < 0) { printk("ERROR: remote %d\n", ret); return ret; }

      printk("[TMP421] sample=%d local=%d.%03d C remote=%d.%03d C\n",
             i + 1,
             (int)(local_mc / 1000),
             (int)(local_mc >= 0 ? local_mc % 1000 : (-local_mc) % 1000),
             (int)(remote_mc / 1000),
             (int)(remote_mc >= 0 ? remote_mc % 1000 : (-remote_mc) % 1000));

      k_msleep(1000);
    }

  return 0;
}
