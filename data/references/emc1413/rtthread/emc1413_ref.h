/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add EMC1413 driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_EMC1413_H_
#define DRIVERS_INCLUDE_EMC1413_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * EMC1413 的 7 位 I2C 地址。
 * 默认地址取决于具体封装型号，但常见的是 0x4C。
 */
#define EMC1413_ADDR_DEFAULT         0x4C

#define EMC1413_DEFAULT_BUS_NAME     "i2c1"
#define EMC1413_DEFAULT_ADDR         EMC1413_ADDR_DEFAULT

/*
 * EMC1413 寄存器定义（来自数据手册 Table 4-1）。
 */
#define EMC1413_REG_INTERNAL_TEMP_HI 0x00  /* 内部温度高字节 */
#define EMC1413_REG_EXT1_TEMP_HI     0x01  /* 外部通道 1 温度高字节 */
#define EMC1413_REG_EXT2_TEMP_HI     0x23  /* 外部通道 2 温度高字节 */
#define EMC1413_REG_STATUS           0x02  /* 状态寄存器 */
#define EMC1413_REG_CONFIG           0x03  /* 配置寄存器 */
#define EMC1413_REG_CONV_RATE        0x04  /* 转换速率 */
#define EMC1413_REG_INTERNAL_TEMP_LO 0x29  /* 内部温度低字节 */
#define EMC1413_REG_EXT1_TEMP_LO     0x10  /* 外部通道 1 温度低字节 */
#define EMC1413_REG_EXT2_TEMP_LO     0x24  /* 外部通道 2 温度低字节 */
#define EMC1413_REG_PRODUCT_ID       0xFD  /* 产品 ID */
#define EMC1413_REG_MANUFACTURER_ID  0xFE  /* 制造商 ID: 0x5D (SMSC/Microchip) */
#define EMC1413_REG_REVISION         0xFF  /* 版本号 */

/* 制造商 ID */
#define EMC1413_MANUFACTURER_ID      0x5D

/* 产品 ID (EMC1413 = 0x21, EMC1414 = 0x25) */
#define EMC1413_PRODUCT_ID           0x21
#define EMC1414_PRODUCT_ID           0x25

/*
 * 配置寄存器位定义。
 */
#define EMC1413_CONFIG_MASK          (1U << 7) /* 1: 屏蔽 ALERT */
#define EMC1413_CONFIG_RUN_STOP      (1U << 6) /* 0: 运行, 1: 待机 */
#define EMC1413_CONFIG_RANGE         (1U << 2) /* 0: 0~127C, 1: -64~191C */

/*
 * 温度通道枚举。
 */
enum emc1413_channel
{
    EMC1413_CH_INTERNAL = 0,
    EMC1413_CH_EXTERNAL_1,
    EMC1413_CH_EXTERNAL_2,
    EMC1413_CH_COUNT
};

/*
 * EMC1413 驱动设备对象。
 */
struct emc1413_device
{
    struct rt_i2c_bus_device *bus;
    const char *bus_name;
    rt_uint8_t addr;
};

/*
 * 初始化 EMC1413 设备对象。
 */
rt_err_t emc1413_init(struct emc1413_device *dev,
                      const char            *bus_name,
                      rt_uint8_t             addr);

/*
 * 探测 EMC1413 是否在线（验证制造商 ID）。
 */
rt_err_t emc1413_probe(struct emc1413_device *dev);

/*
 * 读取指定通道的温度值，返回毫摄氏度。
 */
rt_err_t emc1413_read_temperature(struct emc1413_device *dev,
                                  enum emc1413_channel   channel,
                                  rt_int32_t            *temp_mcelsius);

/*
 * 设置扩展温度范围（-64~191 C）。
 */
rt_err_t emc1413_set_extended_range(struct emc1413_device *dev,
                                    rt_bool_t              enable);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_EMC1413_H_ */
