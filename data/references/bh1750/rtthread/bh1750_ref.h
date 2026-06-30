/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-15     Lin        refactor BH1750 driver to standard structure
 */
#ifndef DRIVERS_INCLUDE_BH1750_H_
#define DRIVERS_INCLUDE_BH1750_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * BH1750 的 ADDR 引脚只有两种合法接法：
 * 1. 拉低时，7 位 I2C 地址为 0x23
 * 2. 拉高时，7 位 I2C 地址为 0x5C
 */
#define BH1750_ADDR_LOW             0x23
#define BH1750_ADDR_HIGH            0x5C

/* 默认总线名和默认地址。 */
#define BH1750_DEFAULT_BUS_NAME     "i2c1"
#define BH1750_DEFAULT_ADDR         BH1750_ADDR_LOW

/*
 * BH1750 支持的测量模式。
 * 命令值来自 BH1750 数据手册的 Command 一览表。
 */
enum bh1750_measure_mode
{
    BH1750_CONT_H_RES_MODE   = 0x10, /* 连续高分辨率模式，分辨率 1 lx */
    BH1750_CONT_H_RES_MODE2  = 0x11, /* 连续高分辨率模式 2，分辨率 0.5 lx */
    BH1750_CONT_L_RES_MODE   = 0x13, /* 连续低分辨率模式，分辨率 4 lx */
    BH1750_ONE_H_RES_MODE    = 0x20, /* 单次高分辨率模式，测完自动回到掉电 */
    BH1750_ONE_H_RES_MODE2   = 0x21, /* 单次高分辨率模式 2，测完自动回到掉电 */
    BH1750_ONE_L_RES_MODE    = 0x23  /* 单次低分辨率模式，测完自动回到掉电 */
};

/*
 * 这是 BH1750 驱动的设备对象。
 * 它本身不分配硬件资源，只是把“总线、地址、模式”这些信息组织在一起，
 * 方便上层统一调用。
 */
struct bh1750_device
{
    struct rt_i2c_bus_device *bus; /* 已经绑定好的 I2C 总线对象 */
    const char *bus_name;          /* I2C 总线名称，例如 i2c1 */
    rt_uint8_t addr;               /* BH1750 的 7 位 I2C 地址 */
    rt_uint8_t mode;               /* 当前使用的测量模式 */
};

/*
 * 判断一个地址是否是 BH1750 合法支持的地址。
 */
rt_bool_t bh1750_is_valid_address(rt_uint8_t addr);

/*
 * 判断一个模式值是否是 BH1750 支持的测量模式。
 */
rt_bool_t bh1750_is_valid_mode(rt_uint8_t mode);

/*
 * 把模式值转换成便于打印的英文说明。
 * 返回的是静态字符串，不需要释放。
 */
const char *bh1750_mode_to_string(rt_uint8_t mode);

/*
 * 使用默认参数或指定参数初始化一个 BH1750 设备对象。
 * 这个函数只做“软件层绑定”，不会立刻发 I2C 命令。
 */
rt_err_t bh1750_init(struct bh1750_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr);

/*
 * 修改设备对象当前使用的测量模式。
 */
rt_err_t bh1750_set_mode(struct bh1750_device *dev, rt_uint8_t mode);

/*
 * 探测指定地址上是否真的存在 BH1750。
 * 原理是尝试发送一条合法命令，只要设备应答 ACK，就基本说明它在线。
 */
rt_err_t bh1750_probe(struct bh1750_device *dev);

/*
 * 读取一次 BH1750 的原始 16 位测量值。
 */
rt_err_t bh1750_read_raw(struct bh1750_device *dev, rt_uint16_t *raw);

/*
 * 把 BH1750 的原始值换算成 lux * 100。
 * 例如返回 12345，表示 123.45 lux。
 */
rt_uint32_t bh1750_raw_to_lux_x100(rt_uint16_t raw);

/*
 * 直接读取一次换算后的光照度值，单位为 lux * 100。
 */
rt_err_t bh1750_read_lux_x100(struct bh1750_device *dev, rt_uint32_t *lux_x100);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_BH1750_H_ */
