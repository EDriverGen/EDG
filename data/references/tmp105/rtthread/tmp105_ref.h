/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add TMP105 driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_TMP105_H_
#define DRIVERS_INCLUDE_TMP105_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * TMP105 的 7 位 I2C 地址由 A0 引脚决定：
 *   A0 = GND -> 0x48
 *   A0 = V+  -> 0x49
 */
#define TMP105_ADDR_LOW              0x48
#define TMP105_ADDR_HIGH             0x49

/* 默认总线名和默认地址。 */
#define TMP105_DEFAULT_BUS_NAME      "i2c1"
#define TMP105_DEFAULT_ADDR          TMP105_ADDR_LOW

/*
 * TMP105 的寄存器指针值。
 * 与 LM75 系列的寄存器布局一致。
 */
#define TMP105_REG_TEMPERATURE       0x00
#define TMP105_REG_CONFIG            0x01
#define TMP105_REG_T_LOW             0x02
#define TMP105_REG_T_HIGH            0x03

/*
 * Configuration Register 的各个位定义。
 * 来自 TMP105 数据手册 Table 6。
 */
#define TMP105_CONF_SHUTDOWN         (1U << 0)
#define TMP105_CONF_THERMOSTAT_MODE  (1U << 1) /* 0: comparator, 1: interrupt */
#define TMP105_CONF_POLARITY         (1U << 2) /* 0: ALERT 低有效, 1: 高有效 */
#define TMP105_CONF_FAULT_QUEUE_0    (1U << 3)
#define TMP105_CONF_FAULT_QUEUE_1    (1U << 4)
#define TMP105_CONF_RES_0            (1U << 5) /* 转换分辨率：00=9bit, 11=12bit */
#define TMP105_CONF_RES_1            (1U << 6)
#define TMP105_CONF_ONE_SHOT         (1U << 7)

/*
 * TMP105 温度分辨率由 R1R0 位决定，最高 12 位（0.0625 C）。
 * 驱动默认使用 12 位分辨率。
 * 以毫摄氏度作为上层接口单位。
 */
#define TMP105_TEMP_MC_MIN           (-55000)
#define TMP105_TEMP_MC_MAX           128000
#define TMP105_TEMP_STEP_MC          63   /* 62.5 mC, 取整到 63 用于简化计算 */

/*
 * TMP105 驱动设备对象。
 */
struct tmp105_device
{
    struct rt_i2c_bus_device *bus;
    const char *bus_name;
    rt_uint8_t addr;
};

/*
 * 判断地址是否合法。
 */
rt_bool_t tmp105_is_valid_address(rt_uint8_t addr);

/*
 * 初始化 TMP105 设备对象。
 */
rt_err_t tmp105_init(struct tmp105_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr);

/*
 * 探测 TMP105 是否在线。
 */
rt_err_t tmp105_probe(struct tmp105_device *dev);

/*
 * 读取温度值，返回毫摄氏度。
 */
rt_err_t tmp105_read_temperature(struct tmp105_device *dev,
                                 rt_int32_t           *temp_mcelsius);

/*
 * 设置转换分辨率（9/10/11/12 位）。
 */
rt_err_t tmp105_set_resolution(struct tmp105_device *dev, rt_uint8_t bits);

/*
 * 读取 Configuration Register。
 */
rt_err_t tmp105_read_config(struct tmp105_device *dev, rt_uint8_t *config);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_TMP105_H_ */
