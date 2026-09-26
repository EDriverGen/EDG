/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Sample for XiUOS
 */
#include <stdio.h>
#include <transform.h>
#include "lsm303dlhc_ref.h"

#define I2C_DEV_PATH  "/dev/i2c1"

int lsm303dlhc_sample(void)
{
  struct lsm303dlhc_device dev;
  struct lsm303dlhc_accel_data accel;
  struct lsm303dlhc_mag_data mag;
  int ret;
  int i;

  ret = lsm303dlhc_init(&dev, I2C_DEV_PATH);
  if (ret < 0) { printf("ERROR: init failed\n"); return -1; }

  ret = lsm303dlhc_probe(&dev);
  if (ret < 0) { printf("ERROR: probe failed\n"); lsm303dlhc_deinit(&dev); return -1; }

  printf("[LSM303DLHC] accel=0x%02X mag=0x%02X probe OK\n",
         LSM303DLHC_ACCEL_ADDR, LSM303DLHC_MAG_ADDR);

  lsm303dlhc_accel_start(&dev);
  lsm303dlhc_mag_start(&dev);
  PrivTaskDelay(100);

  for (i = 0; i < 5; i++)
    {
      ret = lsm303dlhc_read_accel(&dev, &accel);
      if (ret < 0) { printf("ERROR: accel read failed\n"); break; }

      ret = lsm303dlhc_read_mag(&dev, &mag);
      if (ret < 0) { printf("ERROR: mag read failed\n"); break; }

      printf("[LSM303DLHC] sample=%d accel=(%d,%d,%d) mag=(%d,%d,%d)\n",
             i + 1, accel.x, accel.y, accel.z, mag.x, mag.y, mag.z);

      PrivTaskDelay(500);
    }

  lsm303dlhc_deinit(&dev);
  return 0;
}
