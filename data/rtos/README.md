# RTOS Sources

Download the RTOS source trees into the relative paths recorded in `manifest.json`.
The pipeline reads local files only; it does not download repositories at runtime.

| RTOS | Source |
| --- | --- |
| FreeRTOS | https://github.com/FreeRTOS/FreeRTOS.git |
| Apache NuttX | https://github.com/apache/nuttx.git and https://github.com/apache/nuttx-apps.git |
| RT-Thread | https://github.com/RT-Thread/rt-thread.git |
| XiUOS | https://www.gitlink.org.cn/xuos/xiuos.git |
| Zephyr | https://github.com/zephyrproject-rtos/zephyr.git |
| Eclipse ThreadX | https://github.com/eclipse-threadx/threadx.git |
| RIOT OS | https://github.com/RIOT-OS/RIOT.git |
| ChibiOS | https://github.com/ChibiOS/ChibiOS.git |
| TobudOS | https://gitee.com/tobudos/kernel.git, https://atomgit.com/tobudos/ChipAdaptation.git, and https://atomgit.com/tobudos/Document.git |
| OpenHarmony LiteOS-M | https://gitee.com/openharmony/kernel_liteos_m.git and https://gitee.com/openharmony/drivers_hdf_core.git |
| CMSIS-RTX | https://github.com/ARM-software/CMSIS-RTX.git |
| Apache Mynewt | https://github.com/apache/mynewt-core.git |
| RTEMS | https://gitlab.rtems.org/rtems/rtos/rtems.git |

For board bundles, also place the referenced vendor SDK or middleware under the path shown in `manifest.json`.
