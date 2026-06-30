/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-15     Lin          add LM75A driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_LM75A_H_
#define DRIVERS_INCLUDE_LM75A_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * LM75A 的 7 位 I2C 地址格式是：
 * 1001 A2 A1 A0
 *
 * 因此只要 A2/A1/A0 的接法不同，合法地址范围就是 0x48 ~ 0x4F。
 * 你当前模块的示例代码里使用的是 0x90/0x91 这种 8 位地址，
 * 换算成 7 位地址后正好是 0x48，所以这里把 0x48 作为默认地址。
 */
#define LM75A_ADDR_MIN               0x48
#define LM75A_ADDR_MAX               0x4F

/* 默认总线名和默认地址。 */
#define LM75A_DEFAULT_BUS_NAME       "i2c1"
#define LM75A_DEFAULT_ADDR           LM75A_ADDR_MIN

/*
 * LM75A 的寄存器指针值。
 * 读写数据时，先把寄存器号写给芯片，再进行真正的数据读写。
 */
#define LM75A_REG_TEMP               0x00
#define LM75A_REG_CONF               0x01
#define LM75A_REG_THYST              0x02
#define LM75A_REG_TOS                0x03

/*
 * Configuration Register 的各个位定义。
 * 这些位的含义来自数据手册的配置寄存器说明。
 */
#define LM75A_CONF_SHUTDOWN          (1U << 0) /* 1: 进入 shutdown 模式 */
#define LM75A_CONF_OS_COMP_INT       (1U << 1) /* 0: comparator, 1: interrupt */
#define LM75A_CONF_OS_POLARITY       (1U << 2) /* 0: OS 低有效, 1: OS 高有效 */
#define LM75A_CONF_FAULT_QUEUE_0     (1U << 3)
#define LM75A_CONF_FAULT_QUEUE_1     (1U << 4)
#define LM75A_CONF_OS_OPERATION      (1U << 5)

/*
 * LM75A 在标准模式下的温度范围大约是 -55 C 到 +125 C。
 * 为了方便 MCU 侧做整数运算，这里统一使用“毫摄氏度”作为工程单位：
 * 25000 表示 25.000 C，-125 表示 -0.125 C。
 */
#define LM75A_TEMP_MC_MIN            (-55000)
#define LM75A_TEMP_MC_MAX            125000

/*
 * LM75A 的温度分辨率是 0.125 C，也就是 125 mC。
 * 驱动内部把原始值定义成“单位为 0.125 C 的有符号整数”。
 */
#define LM75A_TEMP_STEP_MC           125

/*
 * 这是 LM75A 驱动的设备对象。
 * 它只保存总线和地址这些软件层面的绑定信息，
 * 不直接拥有硬件资源，也不需要单独的动态内存分配。
 */
struct lm75a_device
{
    struct rt_i2c_bus_device *bus; /* 已绑定的 I2C 总线对象 */
    const char *bus_name;          /* I2C 总线名称，例如 i2c1 */
    rt_uint8_t addr;               /* LM75A 的 7 位 I2C 地址 */
};

/*
 * 判断某个地址是否在 LM75A 的合法地址范围内。
 */
rt_bool_t lm75a_is_valid_address(rt_uint8_t addr);

/*
 * 初始化 LM75A 设备对象。
 * 这个函数只做“找到总线并保存参数”这件事，不会立即访问芯片。
 */
rt_err_t lm75a_init(struct lm75a_device *dev,
                    const char          *bus_name,
                    rt_uint8_t           addr);

/*
 * 探测指定地址上是否真的有 LM75A 在线。
 * 这里的做法是读取 Configuration Register；
 * 只要能收到正常 ACK 并读回 1 字节数据，就基本说明设备存在。
 */
rt_err_t lm75a_probe(struct lm75a_device *dev);

/*
 * 读取和写入 Configuration Register。
 */
rt_err_t lm75a_read_config(struct lm75a_device *dev, rt_uint8_t *config);
rt_err_t lm75a_write_config(struct lm75a_device *dev, rt_uint8_t config);

/*
 * 便捷接口：打开或关闭 shutdown 模式。
 * 本质上是对配置寄存器做一次“读-改-写”。
 */
rt_err_t lm75a_set_shutdown(struct lm75a_device *dev, rt_bool_t enable);

/*
 * 读取一次温度寄存器的原始值。
 * 返回值 raw 的单位是 0.125 C。
 * 例如：
 * raw = 200  表示 25.000 C
 * raw = -1   表示 -0.125 C
 */
rt_err_t lm75a_read_raw(struct lm75a_device *dev, rt_int16_t *raw);

/*
 * 把原始值换算成毫摄氏度。
 * 例如 raw = 200 时，返回 25000。
 */
rt_int32_t lm75a_raw_to_mcelsius(rt_int16_t raw);

/*
 * 直接读取换算后的温度值，单位为毫摄氏度。
 */
rt_err_t lm75a_read_temp_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius);

/*
 * 读取/写入 T_HYST 和 T_OS 阈值寄存器，单位均为毫摄氏度。
 * 写入时如果不是 0.125 C 的整数倍，驱动会自动四舍五入到最近的可表示值。
 */
rt_err_t lm75a_read_thyst_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius);
rt_err_t lm75a_write_thyst_mcelsius(struct lm75a_device *dev, rt_int32_t temp_mcelsius);
rt_err_t lm75a_read_tos_mcelsius(struct lm75a_device *dev, rt_int32_t *temp_mcelsius);
rt_err_t lm75a_write_tos_mcelsius(struct lm75a_device *dev, rt_int32_t temp_mcelsius);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_LM75A_H_ */
