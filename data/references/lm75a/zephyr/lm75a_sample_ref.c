/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LM75A Temperature Sensor Sample for Zephyr
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include "lm75a_ref.h"

#define SAMPLE_COUNT   5
#define SAMPLE_INTERVAL_MS 1000

/*
 * Obtain the I2C bus device from the device tree.
 * Adjust the label to match your board's DT (e.g., &i2c0, &i2c1).
 */
#define I2C_NODE DT_NODELABEL(i2c1)

int main(void)
{
  const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
  struct lm75a_device dev;
  int32_t temp_mc;
  int ret;

  if (!device_is_ready(i2c_dev))
    {
      printk("ERROR: I2C bus not ready\n");
      return -1;
    }

  ret = lm75a_init(&dev, i2c_dev, LM75A_DEFAULT_ADDR);
  if (ret < 0)
    {
      printk("ERROR: lm75a_init failed: %d\n", ret);
      return ret;
    }

  ret = lm75a_probe(&dev);
  if (ret < 0)
    {
      printk("ERROR: LM75A not found at 0x%02X: %d\n",
             LM75A_DEFAULT_ADDR, ret);
      return ret;
    }

  printk("[LM75A] addr=0x%02X probe OK\n", LM75A_DEFAULT_ADDR);

  for (int i = 0; i < SAMPLE_COUNT; i++)
    {
      ret = lm75a_read_temperature(&dev, &temp_mc);
      if (ret < 0)
        {
          printk("ERROR: read temperature failed: %d\n", ret);
          return ret;
        }

      printk("[LM75A] sample=%d temp=%d.%03d C\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)(temp_mc >= 0 ? temp_mc % 1000 : (-temp_mc) % 1000));

      if (i + 1 < SAMPLE_COUNT)
        {
          k_msleep(SAMPLE_INTERVAL_MS);
        }
    }

  return 0;
}
