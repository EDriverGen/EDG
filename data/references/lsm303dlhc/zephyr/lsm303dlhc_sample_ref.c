/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include "lsm303dlhc_ref.h"

#define I2C_NODE DT_NODELABEL(i2c1)

int main(void)
{
  const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
  struct lsm303dlhc_device dev;
  struct lsm303dlhc_accel_data accel;
  struct lsm303dlhc_mag_data mag;
  int ret;

  if (!device_is_ready(i2c_dev))
    {
      printk("ERROR: I2C bus not ready\n");
      return -1;
    }

  ret = lsm303dlhc_init(&dev, i2c_dev);
  if (ret < 0) { printk("ERROR: init %d\n", ret); return ret; }

  ret = lsm303dlhc_probe(&dev);
  if (ret < 0) { printk("ERROR: probe %d\n", ret); return ret; }

  printk("[LSM303DLHC] accel=0x%02X mag=0x%02X probe OK\n",
         LSM303DLHC_ACCEL_ADDR, LSM303DLHC_MAG_ADDR);

  lsm303dlhc_accel_start(&dev);
  lsm303dlhc_mag_start(&dev);

  k_msleep(100);

  for (int i = 0; i < 5; i++)
    {
      ret = lsm303dlhc_read_accel(&dev, &accel);
      if (ret < 0) { printk("ERROR: accel %d\n", ret); return ret; }

      ret = lsm303dlhc_read_mag(&dev, &mag);
      if (ret < 0) { printk("ERROR: mag %d\n", ret); return ret; }

      printk("[LSM303DLHC] sample=%d accel=(%d,%d,%d) mag=(%d,%d,%d)\n",
             i + 1, accel.x, accel.y, accel.z, mag.x, mag.y, mag.z);

      k_msleep(500);
    }

  return 0;
}
