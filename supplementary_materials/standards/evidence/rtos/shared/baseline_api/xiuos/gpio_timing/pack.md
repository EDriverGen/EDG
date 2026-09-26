# Raw RTOS/Bus Pack: xiuos / gpio_timing

Selection mode: `curated`.
This pack contains raw RTOS source/header/documentation excerpts only.
It excludes DriverGen contracts, IRs, reference drivers, oracle data,
expected transactions, and generated evaluation reports.

## Source: `data/rtos/xiuos/APP_Framework/Framework/transform_layer/xizi/transform.h`

```c
/*
* Copyright (c) 2020 AIIT XUOS Lab
* XiUOS is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*        http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

/**
 * @file transform.h
 * @brief Interface function declarations required by the framework
 * @version 1.0
 * @author AIIT XUOS Lab
 * @date 2021.06.04
 */

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <timer.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <user_api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OPE_INT                 0x0000
#define OPE_CFG                 0x0001

#define OPER_WDT_SET_TIMEOUT    0x0002
#define OPER_WDT_KEEPALIVE      0x0003

#define NAME_NUM_MAX            32

#define LINKLIST_FLAG_FIFO      0x00           
#define LINKLIST_FLAG_PRIO      0x01

#ifndef EVENT_AND
#define EVENT_AND          (1 << 0)
#endif
#ifndef EVENT_OR
#define EVENT_OR           (1 << 1)
#endif
#ifndef EVENT_AUTOCLEAN
#define EVENT_AUTOCLEAN    (1 << 2)
#endif

/*********************GPIO define*********************/
#define GPIO_LOW    0x00
#define GPIO_HIGH   0x01

#define GPIO_CFG_OUTPUT             0x00
#define GPIO_CFG_INPUT              0x01
#define GPIO_CFG_INPUT_PULLUP       0x02
#define GPIO_CFG_INPUT_PULLDOWN     0x03
#define GPIO_CFG_OUTPUT_OD          0x04

#define GPIO_CONFIG_MODE            0xffffffff

/********************SERIAL define*******************/
#define BAUD_RATE_2400          2400
#define BAUD_RATE_4800          4800
#define BAUD_RATE_9600          9600
#define BAUD_RATE_19200         19200
#define BAUD_RATE_38400         38400
#define BAUD_RATE_57600         57600
#define BAUD_RATE_115200        115200
#define BAUD_RATE_230400        230400
#define BAUD_RATE_460800        460800
#define BAUD_RATE_921600        921600
#define BAUD_RATE_2000000       2000000
#define BAUD_RATE_3000000       3000000

#define DATA_BITS_5             5
#define DATA_BITS_6             6
#define DATA_BITS_7             7
#define DATA_BITS_8             8
#define DATA_BITS_9             9

#define STOP_BITS_1             1
#define STOP_BITS_2             2
#define STOP_BITS_3             3
#define STOP_BITS_4             4

#define PARITY_NONE             1
#define PARITY_ODD              2
#define PARITY_EVEN             3

#define BIT_ORDER_LSB           1
#define BIT_ORDER_MSB           2

#define NRZ_NORMAL              1
#define NRZ_INVERTED            2

#ifndef SERIAL_RB_BUFSZ
#define SERIAL_RB_BUFSZ         128
#endif

/********************SPI define*******************/
#define SPI_MAX_CLOCK            40000000
#define spi_device_max_num       4

#define SPI_LINE_CPHA            (1 << 0)                           
#define SPI_LINE_CPOL            (1 << 1)                          

#define SPI_LSB                  (0 << 2)                             
#define SPI_MSB                  (1 << 2)                             

#define SPI_DEV_MASTER           (0 << 3)                            
#define SPI_DEV_SLAVE            (1 << 3)      

#define SPI_MODE_0               (0 | 0)                        
#define SPI_MODE_1               (0 | SPI_LINE_CPHA)              
#define SPI_MODE_2               (SPI_LINE_CPOL | 0)            
#define SPI_MODE_3               (SPI_LINE_CPOL | SPI_LINE_CPHA)    
#define SPI_MODE_MASK            (SPI_LINE_CPHA | SPI_LINE_CPOL | SPI_MSB)

#define SPI_CS_HIGH              (1 << 4)                            
#define SPI_NO_CS                (1 << 5)                           
#define SPI_3WIRE                (1 << 6)                             
#define SPI_READY                (1 << 7)

struct PinDevIrq
{
    int irq_mode;//< RISING/FALLING/HIGH/LOW
    void (*hdr) (void *args);//< callback function
    void *args;//< the params of callback function
};

struct PinParam
{
    int cmd;//< cmd:GPIO_CONFIG_MODE/GPIO_IRQ_REGISTER/GPIO_IRQ_FREE/GPIO_IRQ_DISABLE/GPIO_IRQ_ENABLE
    long  pin;//< pin number
    int mode;//< pin mode: input/output
    struct PinDevIrq irq_set;//< pin irq set
    uint64_t arg;
};

struct PinStat
{
    long pin;//< pin number
    uint16_t val;//< pin level
};

enum ExtSerialPortConfigure
{
    PORT_CFG_INIT = 0,
    PORT_CFG_PARITY_CHECK,
    PORT_CFG_DISABLE,
    PORT_CFG_DIV,
};

struct SerialDataCfg
{
    uint32_t serial_baud_rate;
    uint8_t serial_data_bits;
    uint8_t serial_stop_bits;
    uint8_t serial_parity_mode;
    uint8_t serial_bit_order;
    uint8_t serial_invert_mode;
    uint16_t serial_buffer_size;
    int32 serial_timeout;

    int (*dev_recv_callback) (void *dev, size_t length);

    uint8_t is_ext_uart;
    uint8_t ext_uart_no;
    enum ExtSerialPortConfigure port_configure;
};

struct SpiMasterParam
{
    uint8 spi_work_mode;//CPOL CPHA
    uint8 spi_frame_format;//frame format
    uint8 spi_data_bit_width;//bit width
    uint8 spi_data_endian;//little endian：0，big endian：1
    uint32 spi_maxfrequency;//work frequency
};

enum IoctlDriverType
{
    SERIAL_TYPE = 0,
    SPI_TYPE,
    I2C_TYPE,
    PIN_TYPE,
    LCD_TYPE,
    ADC_TYPE,
    DAC_TYPE,
    WDT_TYPE,
    RTC_TYPE,
    CAMERA_TYPE,
    CAN_TYPE,
    KPU_TYPE,
    FLASH_TYPE,
    TIME_TYPE,
    DEFAULT_TYPE,
};


struct DvpRegConfigureInfo
{
    uint8_t device_addr;
    uint16_t reg_addr;
    uint8_t reg_value;
} ;

struct PrivIoctlCfg
{
    enum IoctlDriverType ioctl_driver_type;
    void *args;
};

typedef struct 
{
    uint16 x_pos;
    uint16 y_pos;
    uint16 width;
    uint16 height;
    uint8  font_size;
    uint8 *addr;
    uint16 font_color;
    uint16 back_color;
}LcdStringParam;

typedef struct 
{
    uint16 x_startpos;
    uint16 x_endpos;
    uint16 y_startpos;
    uint16 y_endpos;
    void* pixel_color;
}LcdPixelParam;

struct CameraCfg
{
    uint16_t window_w;
    uint16_t window_h;
    uint16_t window_xoffset;
    uint16_t window_yoffset;
    uint16_t output_w;
    uint16_t output_h;
    uint8_t gain;
    uint8_t gain_manu_enable;
};

typedef struct 
{
    char type; // 0:write string;1:write dot
    LcdPixelParam pixel_info;
    LcdStringParam string_info;
}LcdWriteParam;

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t press;
}TouchDataParam;

struct TouchDataStandard
{
    uint16 x;
    uint16 y;
};

struct RtcDrvConfigureParam
{
    int rtc_operation_cmd;
    time_t *time;
};

typedef struct 
{
    uintptr_t pdata; 
    uint32_t length;
}_ioctl_shoot_para;

typedef struct 
{
    uint32_t width;         // width   The width  of image
    uint32_t height;        // height  The height of image
}_ioctl_set_reso;

typedef struct 
{
    uintptr_t r_addr;
    uintptr_t g_addr;
    uintptr_t b_addr;
}RgbAddress;

enum TCP_OPTION {
    SEND_DATA = 0,
    RECV_DATA,
};

struct CanDriverConfigure 
{
    uint8 tsjw;
    uint8 tbs2 ;
    uint8 tbs1;
    uint8 mode;
    uint16 brp;
};

struct CanSendConfigure
{
    uint32 stdid;
    uint32 exdid;
    uint8 ide;
    uint8 rtr;
    uint8 data_lenth;
    uint8 *data;
};

typedef struct
{
    uint8_t *buffer;
    size_t length;
}KpuOutputBuffer;

#define PRIV_SYSTICK_GET (CurrentTicksGain())
#define PRIV_LCD_DEV "/dev/lcd_dev"
#define MY_DISP_HOR_RES BSP_LCD_Y_MAX
#define MY_DISP_VER_RES BSP_LCD_X_MAX

#define PRIV_TOUCH_DEV "/dev/touch_dev"
#define MY_INDEV_X BSP_LCD_Y_MAX
#define MY_INDEV_Y BSP_LCD_X_MAX

#define LCD_STRING_TYPE 0
#define LCD_DOT_TYPE 1
#define LCD_SIZE 320
#define IMAGE_HEIGHT 240
#define IMAGE_WIDTH 320
#define NULL_PARAMETER 0

#define REG_SCCB_READ 0xA2U
#define REG_SCCB_WRITE 0xA3U
#define SCCB_REG_LENGTH 0x08U

#define SET_DISPLAY_ADDR (0xD1)
#define SET_AI_ADDR (0xD2)
#define FLAG_CHECK (0xD4)

#define LOAD_KMODEL 0xA0
#define RUN_KMODEL 0xA1
#define GET_OUTPUT 0xA2
#define WAIT_FLAG 0xA3

#define IOCTRL_CAMERA_START_SHOT            (22)     // start shoot
#define IOCTRL_CAMERA_OUT_SIZE_RESO (23)
#define IOCTRL_CAMERA_SET_WINDOWS_SIZE      (21)     // user set specific windows outsize
#define IOCTRL_CAMERA_SET_LIGHT             (24)     //set light mode
#define IOCTRL_CAMERA_SET_COLOR             (25)     //set color saturation
#define IOCTRL_CAMERA_SET_BRIGHTNESS        (26)     //set color brightness
#define IOCTRL_CAMERA_SET_CONTRAST          (27)     //set contrast
#define IOCTRL_CAMERA_SET_EFFECT            (28)     //set effect
#define IOCTRL_CAMERA_SET_EXPOSURE          (29)     //set auto exposure
/*********************shell***********************/

#ifndef SEPARATE_COMPILE
//for int func(int argc, char *agrv[])
#define PRIV_SHELL_CMD_MAIN_ATTR (SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN))

//for int func(int i, char ch, char *str)
#define PRIV_SHELL_CMD_FUNC_ATTR (SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC))

/**
 * @brief Priv-shell Command definition 
 * 
 * @param _func Command function 
 * @param _desc Command description 
 * @param _attr Command attributes if need
 */
#define PRIV_SHELL_CMD_FUNCTION(_func, _desc, _attr)  \
    SHELL_EXPORT_CMD(_attr, _func, _func, _desc)

#else
//for int func(int argc, char *agrv[])
#define PRIV_SHELL_CMD_MAIN_ATTR() 

//for int func(int i, char ch, char *str)
#define PRIV_SHELL_CMD_FUNC_ATTR()
#define PRIV_SHELL_CMD_FUNCTION(_func, _desc, _attr) 

#endif
/**********************mutex**************************/

int PrivMutexCreate(pthread_mutex_t *p_mutex, const pthread_mutexattr_t *attr);
int PrivMutexDelete(pthread_mutex_t *p_mutex);
int PrivMutexObtain(pthread_mutex_t *p_mutex);
int PrivMutexAbandon(pthread_mutex_t *p_mutex);

/*********************semaphore**********************/

int PrivSemaphoreCreate(sem_t *sem, int pshared, unsigned int value);
int PrivSemaphoreDelete(sem_t *sem);
int PrivSemaphoreObtainWait(sem_t *sem, const struct timespec *abstime);
int PrivSemaphoreObtainNoWait(sem_t *sem);
int PrivSemaphoreAbandon(sem_t *sem);
int32_t PrivSemaphoreSetValue(int32_t sem, uint16_t val);

/*********************event**********************/
#ifndef SEPARATE_COMPILE
int PrivEventCreate(uint8_t flag);
int PrivEvenDelete(int event);
int PrivEvenTrigger(int event, uint32_t set);
int PrivEventProcess(int event, uint32_t set, uint8_t option, int32_t wait_time, unsigned int *Recved);
#endif

/*********************task**************************/

int PrivTaskCreate(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
int PrivTaskStartup(pthread_t *thread);
int PrivTaskDelete(pthread_t thread, int sig);
void PrivTaskQuit(void *value_ptr);
int PrivTaskDelay(int32_t ms);
int PrivUserTaskSearch(void);
uint32_t PrivGetTickTime();

/*********************Files**************************/
int PrivLseek(int fd, off_t offset, int whence);
int PrivFsync(int fd);
int PrivFstat(int fd, struct stat *buf);
int PrivStat(const char *path, struct stat *buf);
int PrivUnlink(const char *path);
char *PrivGetcwd(char *buf, size_t size);
/*********************driver*************************/

int PrivOpen(const char *path, int flags, ...);
int PrivRead(int fd, void *buf, size_t len);
int PrivWrite(int fd, const void *buf, size_t len);
int PrivClose(int fd);
int PrivIoctl(int fd, int cmd, void *args);

/*********************memory***********************/

void *PrivMalloc(size_t size);
void *PrivRealloc(void *pointer, size_t size);
void *PrivCalloc(size_t  count, size_t size);
void PrivFree(void *pointer);

/******************soft timer*********************/
int PrivTimerCreate(clockid_t clockid, struct sigevent * evp, timer_t * timerid);
int PrivTimerDelete(timer_t timerid);
int PrivTimerStartRun(timer_t timerid);
int PrivTimerQuitRun(timer_t timerid);
int PrivTimerModify(timer_t timerid, int flags, const struct itimerspec *restrict value,
                  struct itimerspec *restrict ovalue);

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/xiuos/APP_Framework/Framework/transform_layer/xizi/user_api/switch_api/user_api.h`

```c
/*
 * Copyright (c) 2020 AIIT XUOS Lab
 * XiUOS  is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *        http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

/**
* @file:    user_api.h
* @brief:   the priviate user api for application
* @version: 1.0
* @author:  AIIT XUOS Lab
* @date:    2020/4/20
*
*/

#ifndef XS_USER_API_H
#define XS_USER_API_H

#include <xsconfig.h>
#include <stddef.h>
#include <stdint.h>
#include <libc.h>


#ifdef  SEPARATE_COMPILE

#include "../../../../../../Ubiquitous/XiZi_IIoT/arch/kswitch.h"

#define TASK_INFO      1
#define MEM_INFO       2
#define SEM_INFO       3
#define EVENT_INFO     4
#define MUTEX_INFO     5
#define MEMPOOL_INFO   6
#define MSGQUEUE_INFO  7
#define DEVICE_INFO    8
#define TIMER_INFO     9

int UserPrintInfo(unsigned long i);

struct utask
{
	char        name[NAME_NUM_MAX];         
    void        *func_entry;                
    void        *func_param;     
    int32_t     stack_size;  
    uint8_t     prio; 
};
typedef struct utask UtaskType;

typedef void DIR;

int32_t UserTaskCreate(UtaskType utask);

long UserTaskStartup(int32_t id);
long UserTaskDelete(int32_t id);
long UserTaskSearch(void);
void UserTaskQuit(void);
long UserTaskDelay(int32_t ms);
long UserGetTaskName(int32_t id ,char *name);
int32_t UserGetTaskID(void);
uint8_t UserGetTaskStat(int32_t id);

#ifdef ARCH_SMP
long UserTaskCoreCombine(int32_t id,uint8_t core_id);
long UserTaskCoreUnCombine(int32_t id);
uint8_t UserGetTaskCombinedCore(int32_t id);
uint8_t UserGetTaskRunningCore(int32_t id);
#endif

long UserGetTaskErrorstatus(int32_t id);
uint8_t UserGetTaskPriority(int32_t id);


void *UserMalloc(size_t size);
void *UserRealloc(void *pointer, size_t size);
void *UserCalloc(size_t  count, size_t size);
void UserFree(void *pointer);

#ifdef KERNEL_MUTEX
int32_t UserMutexCreate();
void UserMutexDelete(int32_t mutex);
int32_t UserMutexObtain(int32_t mutex, int32_t wait_time);
int32_t UserMutexAbandon(int32_t mutex);
#endif


#ifdef KERNEL_SEMAPHORE
typedef int32  sem_t;
sem_t UserSemaphoreCreate(uint16_t val);
long UserSemaphoreDelete(sem_t sem);
long UserSemaphoreObtain(sem_t sem, int32_t wait_time);
long UserSemaphoreAbandon(sem_t sem);
long UserSemaphoreSetValue(sem_t sem, uint16_t val);
#endif


#ifdef KERNEL_EVENT
typedef int32 EventIdType;
EventIdType UserEventCreate(uint8_t flag);
void UserEventDelete(EventIdType event);
long UserEventTrigger(EventIdType event, uint32_t set);
long UserEventProcess(EventIdType event, uint32_t set, uint8_t option, 
                         int32_t   wait_time, uint32_t *Recved);
long UserEventReinit(EventIdType event);
#endif


#ifdef KERNEL_MESSAGEQUEUE
int32_t UserMsgQueueCreate(size_t   msg_size, size_t   max_msgs);
long UserMsgQueueDelete(int32_t mq );
long UserMsgQueueSendwait(int32_t mq, const void *buffer,
                                      size_t   size, int32_t  wait_time);
long UserMsgQueueSend(int32_t mq, const void *buffer, size_t size);
long UserMsgQueueUrgentSend(int32_t mq, const void *buffer, size_t size);
long UserMsgQueueRecv(int32_t mq, void *buffer, size_t  size,int32_t wait_time);
long UserMsgQueueReinit(int32_t mq);
#endif

#ifdef KERNEL_SOFTTIMER
int32_t UserTimerCreate(const char *name, void (*timeout)(void *parameter), void *parameter, uint32_t time, uint8_t trigger_mode);
long UserTimerDelete(int32_t timer_id);
long UserTimerStartRun(int32_t timer_id);
long UserTimerQuitRun(int32_t timer_id);
long UserTimerModify(int32_t timer_id, uint32_t ticks);
#endif

int open(const char *path, int flags, ...);
int read(int fd, void *buf, size_t len);
int write(int fd, const void *buf, size_t len);
int close(int fd);
int ioctl(int fd, int cmd, void *args);
off_t lseek(int fd, off_t offset, int whence);
int rename(const char *from, const char *to);
int unlink(const char *path);
int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int fsync(int fd);
int ftruncate(int fd, off_t length);

int mkdir(const char *path, mode_t mode);
DIR *opendir(const char *path);
int closedir(DIR *dirp);
struct dirent *readdir(DIR *dirp);
int rmdir(const char *path);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
long telldir(DIR *dirp);
void seekdir(DIR *dirp, off_t offset);
void rewinddir(DIR *dirp);

#ifdef FS_VFS
struct statfs {
    size_t f_bsize;
    size_t f_blocks;
    size_t f_bfree;
};

int statfs(const char *path, struct statfs *buf);

/* NOTE!!!: when cutting out file system, the 'printf' function can not output angthing */
int Userprintf(const char *fmt, ...);

#endif
 

#else

#include <xizi.h>

#ifdef FS_VFS
#include <iot-vfs_posix.h>
#endif

struct utask
{
	char        name[NAME_NUM_MAX];         
    void        *func_entry;                
    void        *func_param;     
    int32_t     stack_size;  
    uint8_t     prio; 
};
typedef struct utask UtaskType;
int32_t UserTaskCreate(UtaskType utask);

#define UserTaskStartup          StartupKTask
#define UserTaskDelete           KTaskDelete
#define UserTaskQuit             KTaskQuit
#define UserTaskDelay            MdelayKTask
#define UserTaskSearch           UTaskSearch

long UserGetTaskName(int32_t id ,char *name);
int32_t UserGetTaskID(void);
uint8_t UserGetTaskStat(int32_t id);

#ifdef ARCH_SMP
#define UserTaskCoreCombine      KTaskCoreCombine
#define UserTaskCoreUnCombine    KTaskCoreUnCombine

uint8_t UserGetTaskCombinedCore(int32_t id);
uint8_t UserGetTaskRunningCore(int32_t id);
#endif

long UserGetTaskErrorstatus(int32_t id);
uint8_t UserGetTaskPriority(int32_t id);

#define UserMalloc               x_malloc
#define UserRealloc              x_realloc
#define UserCalloc               x_calloc
#define UserFree                 x_free

#ifdef KERNEL_MUTEX
#define UserMutexCreate          KMutexCreate
#define UserMutexDelete          KMutexDelete
#define UserMutexObtain          KMutexObtain
#define UserMutexAbandon         KMutexAbandon
#endif


#ifdef KERNEL_SEMAPHORE
#define UserSemaphoreCreate      KSemaphoreCreate
#define UserSemaphoreDelete      KSemaphoreDelete
#define UserSemaphoreObtain      KSemaphoreObtain
#define UserSemaphoreAbandon     KSemaphoreAbandon
#define UserSemaphoreSetValue    KSemaphoreSetValue
#endif

#ifdef KERNEL_EVENT
#define UserEventCreate          KEventCreate
#define UserEventDelete          KEventDelete
#define UserEventTrigger         KEventTrigger
#define UserEventProcess         KEventProcess
#endif

#ifdef KERNEL_MESSAGEQUEUE
#define UserMsgQueueCreate       KCreateMsgQueue
#define UserMsgQueueDelete       KDeleteMsgQueue
#define UserMsgQueueSendwait     KMsgQueueSendwait
#define UserMsgQueueSend         KMsgQueueSend
#define UserMsgQueueUrgentSend   KMsgQueueUrgentSend
#define UserMsgQueueRecv         KMsgQueueRecv
#define UserMsgQueueReinit       KMsgQueueReinit
#endif

#ifdef KERNEL_SOFTTIMER
int32_t UserTimerCreate(const char *name, void (*timeout)(void *parameter), void *parameter, uint32_t time, uint8_t trigger_mode);
long UserTimerDelete(int32_t timer_id);
long UserTimerStartRun(int32_t timer_id);
long UserTimerQuitRun(int32_t timer_id);
long UserTimerModify(int32_t timer_id, uint32_t ticks);
#endif

#define UserPrintf               KPrintf

#endif

#endif
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/include/bus.h`

```c
/*
* Copyright (c) 2020 AIIT XUOS Lab
* XiUOS is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*        http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

/**
* @file bus.h
* @brief define bus driver framework function and common API
* @version 1.0 
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#ifndef BUS_H
#define BUS_H

#include <xizi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OPE_INT                 0x0000
#define OPE_CFG                 0x0001

#define OPER_WDT_SET_TIMEOUT    0x0002
#define OPER_WDT_KEEPALIVE      0x0003

typedef struct Bus *BusType;
typedef struct HardwareDev *HardwareDevType;
typedef struct Driver *DriverType;

/* need to add new bus type in ../tool/shell/letter-shell/cmd.c, ensure ShowBus cmd supported*/
enum BusType_e
{
    TYPE_I2C_BUS = 0,
    TYPE_SPI_BUS,
    TYPE_HWTIMER_BUS,
    TYPE_USB_BUS,
    TYPE_CAN_BUS,
    TYPE_WDT_BUS,
    TYPE_SDIO_BUS,
    TYPE_TOUCH_BUS,
    TYPE_LCD_BUS,
    TYPE_PIN_BUS,
    TYPE_RTC_BUS,
    TYPE_SERIAL_BUS,
    TYPE_ADC_BUS,
    TYPE_DAC_BUS,
    TYPE_CAMERA_BUS,
    TYPE_KPU_BUS,
    TYPE_BUS_END,
};

enum BusState
{
    BUS_INIT = 0,
    BUS_INSTALL,
    BUS_UNINSTALL,
};

enum DevType
{
    TYPE_I2C_DEV = 0,
    TYPE_SPI_DEV,
    TYPE_HWTIMER_DEV,
    TYPE_USB_DEV,
    TYPE_CAN_DEV,
    TYPE_WDT_DEV,
    TYPE_SDIO_DEV,
    TYPE_TOUCH_DEV,
    TYPE_LCD_DEV,
    TYPE_PIN_DEV,
    TYPE_RTC_DEV,
    TYPE_SERIAL_DEV,
    TYPE_ADC_DEV,
    TYPE_DAC_DEV,
    TYPE_CAMERA_DEV,
    TYPE_KPU_DEV,
    TYPE_DEV_END,
};

enum DevState
{
    DEV_INIT = 0,
    DEV_INSTALL,
    DEV_UNINSTALL,
};

enum DriverType_e
{
    TYPE_I2C_DRV = 0,
    TYPE_SPI_DRV,
    TYPE_HWTIMER_DRV,
    TYPE_USB_DRV,
    TYPE_CAN_DRV,
    TYPE_WDT_DRV,
    TYPE_SDIO_DRV,
    TYPE_TOUCH_DRV,
    TYPE_LCD_DRV,
    TYPE_PIN_DRV,
    TYPE_RTC_DRV,
    TYPE_SERIAL_DRV,
    TYPE_ADC_DRV,
    TYPE_DAC_DRV,
    TYPE_CAMERA_DRV,
    TYPE_KPU_DRV,
    TYPE_DRV_END,
};

enum DriverState
{
    DRV_INIT = 0,
    DRV_INSTALL,
    DRV_UNINSTALL,
};

struct BusConfigureInfo
{
    int configure_cmd;
    void *private_data;
};

struct BusBlockReadParam
{
    x_OffPos pos;
    void* buffer;
    x_size_t size;
    x_size_t read_length;
};

struct BusBlockWriteParam
{
    x_OffPos pos;
    const void* buffer;
    x_size_t size;
};

struct HalDevBlockParam
{
    uint32 cmd;
    struct DeviceBlockArrange dev_block;
    struct DeviceBlockAddr *dev_addr;
};

struct HalDevDone
{
    uint32 (*open) (void *dev);
    uint32 (*close) (void *dev);
    uint32 (*write) (void *dev, struct BusBlockWriteParam *write_param);
    uint32 (*read) (void *dev, struct BusBlockReadParam *read_param);
};

struct HardwareDev
{
    int8 dev_name[NAME_NUM_MAX];
    enum DevType dev_type;
    enum DevState dev_state;
    
    const struct HalDevDone *dev_done;

    int (*dev_recv_callback) (void *dev, x_size_t length);
    int (*dev_block_control) (struct HardwareDev *dev, struct HalDevBlockParam *block_param);

    struct Bus *owner_bus;
    void *private_data;

    int32 dev_sem;

    DoubleLinklistType  dev_link;    
};

struct Driver
{
    int8 drv_name[NAME_NUM_MAX];
    enum DriverType_e driver_type;
    enum DriverState driver_state;

    uint32 (*configure)(void *drv, struct BusConfigureInfo *configure_info);

    struct Bus *owner_bus;
    void *private_data;

    DoubleLinklistType  driver_link;    
};

struct Bus
{
    int8 bus_name[NAME_NUM_MAX];
    enum BusType_e bus_type;
    enum BusState bus_state;

    int32 (*match)(struct Driver *driver, struct HardwareDev *device);

    int bus_lock;

    struct HardwareDev *owner_haldev;
    struct Driver *owner_driver;
    
    void *private_data;

    /*manage the drv of the bus*/
    uint8 driver_cnt;
    uint8 bus_drvlink_flag;
    DoubleLinklistType bus_drvlink;

    /*manage the dev of the bus*/
    uint8 haldev_cnt;
    uint8 bus_devlink_flag;
    DoubleLinklistType bus_devlink;

    uint8 bus_cnt;
    uint8 bus_link_flag;
    DoubleLinklistType  bus_link;    
};

/*Register the BUS,manage with the double linklist*/
int BusRegister(struct Bus *bus);

/*Release the BUS framework*/
int BusRelease(struct Bus *bus);

/*Unregister a certain kind of BUS*/
int BusUnregister(struct Bus *bus);

/*Register the driver to the bus*/
int DriverRegisterToBus(struct Bus *bus, struct Driver *driver);

/*Register the device to the bus*/
int DeviceRegisterToBus(struct Bus *bus, struct HardwareDev *device);

/*Delete the driver from the bus*/
int DriverDeleteFromBus(struct Bus *bus, struct Driver *driver);

/*Delete the device from the bus*/
int DeviceDeleteFromBus(struct Bus *bus, struct HardwareDev *device);

/*Find the bus with bus name*/
BusType BusFind(const char *bus_name);

/*Find the driver of cetain bus*/
DriverType BusFindDriver(struct Bus *bus, const char *driver_name);

/*Find the device of certain bus*/
HardwareDevType BusFindDevice(struct Bus *bus, const char *device_name);

/*Dev receive data callback function*/
uint32 BusDevRecvCallback(struct HardwareDev *dev, int (*dev_recv_callback) (void *dev, x_size_t length));

/*Open the device of the bus*/
uint32 BusDevOpen(struct HardwareDev *dev);

/*Close the device of the bus*/
uint32 BusDevClose(struct HardwareDev *dev);

/*Write data to the device*/
uint32 BusDevWriteData(struct HardwareDev *dev, struct BusBlockWriteParam *write_param);

/*Read data from the device*/
uint32 BusDevReadData(struct HardwareDev *dev, struct BusBlockReadParam *read_param);

/*Configure the driver of the bus*/
uint32 BusDrvConfigure(struct Driver *drv, struct BusConfigureInfo *configure_info);

/*Obtain the bus using a certain dev*/
int DeviceObtainBus(struct Bus *bus, struct HardwareDev *dev, const char *drv_name, struct BusConfigureInfo *configure_info);

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/include/bus_pin.h`

```c
/*
* Copyright (c) 2020 AIIT XUOS Lab
* XiUOS is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*        http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

/**
* @file bus_pin.h
* @brief define pin bus and drv function using bus driver framework
* @version 1.0 
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#ifndef BUS_PIN_H
#define BUS_PIN_H

#include <bus.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PinDriver
{
    struct Driver driver;
    uint32 (*configure) (void *drv, struct BusConfigureInfo *ConfigureInfo);
};

struct PinBus
{
    struct Bus bus;

    void *private_data;
};

/*Register the Pin bus*/
int PinBusInit(struct PinBus *pin_bus, const char *bus_name);

/*Register the Pin driver*/
int PinDriverInit(struct PinDriver *pin_driver, const char *driver_name, void *data);

/*Release the Pin device*/
int PinReleaseBus(struct PinBus *pin_bus);

/*Register the Pin driver to the Pin bus*/
int PinDriverAttachToBus(const char *drv_name, const char *bus_name);

/*Register the driver, manage with the double linklist*/
int PinDriverRegister(struct Driver *driver);

/*Find the register driver*/
DriverType PinDriverFind(const char *drv_name, enum DriverType_e drv_type);

/*Get the initialized Pin bus*/
BusType PinBusInitGet(void);

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/include/dev_pin.h`

```c
/*
* Copyright (c) 2020 AIIT XUOS Lab
* XiUOS is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*        http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

/**
* @file dev_pin.h
* @brief define pin dev function using bus driver framework
* @version 1.0 
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#ifndef DEV_PIN_H
#define DEV_PIN_H

#include <bus.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_LOW                         0x00
#define GPIO_HIGH                        0x01

#define GPIO_CFG_OUTPUT                  0x00
#define GPIO_CFG_INPUT                   0x01
#define GPIO_CFG_INPUT_PULLUP            0x02
#define GPIO_CFG_INPUT_PULLDOWN          0x03
#define GPIO_CFG_OUTPUT_OD               0x04

#define GPIO_IRQ_EDGE_RISING             0x00
#define GPIO_IRQ_EDGE_FALLING            0x01
#define GPIO_IRQ_EDGE_BOTH               0x02
#define GPIO_IRQ_LEVEL_HIGH              0x03
#define GPIO_IRQ_LEVEL_LOW               0x04

#define GPIO_CONFIG_MODE                 0xffffffff
#define GPIO_IRQ_REGISTER                0xfffffffe
#define GPIO_IRQ_FREE                    0xfffffffd
#define GPIO_IRQ_DISABLE                 0xfffffffc
#define GPIO_IRQ_ENABLE                  0xfffffffb

struct PinDevIrq
{
    int irq_mode;//< RISING/FALLING/HIGH/LOW
    void (*hdr) (void *args);//< callback function
    void *args;//< the params of callback function
};

struct PinParam
{
    int cmd;//< cmd:GPIO_CONFIG_MODE/GPIO_IRQ_REGISTER/GPIO_IRQ_FREE/GPIO_IRQ_DISABLE/GPIO_IRQ_ENABLE
    x_base  pin;//< pin number
    int mode;//< pin mode: input/output
    struct PinDevIrq irq_set;//< pin irq set
    uint64 arg;
};

struct PinStat
{
    x_base pin;//< pin number
    uint16 val;//< pin level
};

struct PinIrqHdr
{
    int16 pin;
    uint16 mode;
    void (*hdr) (void *args);
    void *args;
};

struct PinDevDone
{
    uint32 (*open) (void *dev);
    uint32 (*close) (void *dev);
    uint32 (*write) (void *dev, struct BusBlockWriteParam *write_param);
    uint32 (*read) (void *dev, struct BusBlockReadParam *read_param);
};

struct PinHardwareDevice
{
    struct HardwareDev haldev;

    const struct PinDevDone *dev_done;
    
    void *private_data;
};

/*Register the Pin device*/
int PinDeviceRegister(struct PinHardwareDevice *pin_device, void *pin_param, const char *device_name);

/*Register the Pin Device to the Pin bus*/
int PinDeviceAttachToBus(const char *dev_name, const char *bus_name);

/*Find the register Pin device*/
HardwareDevType PinDeviceFind(const char *dev_name, enum DevType dev_type);

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/pin/bus_pin.c`

```c
/*
* Copyright (c) 2020 AIIT XUOS Lab
* XiUOS is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*        http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

/**
* @file bus_pin.c
* @brief register pin bus function using bus driver framework
* @version 1.0 
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#include <bus_pin.h>
#include <dev_pin.h>

int PinBusInit(struct PinBus *pin_bus, const char *bus_name)
{
    NULL_PARAM_CHECK(pin_bus);
    NULL_PARAM_CHECK(bus_name);

    x_err_t ret = EOK;

    if (BUS_INSTALL != pin_bus->bus.bus_state) {
        strncpy(pin_bus->bus.bus_name, bus_name, NAME_NUM_MAX);

        pin_bus->bus.bus_type = TYPE_PIN_BUS;
        pin_bus->bus.bus_state = BUS_INSTALL;
        pin_bus->bus.private_data = pin_bus->private_data;

        ret = BusRegister(&pin_bus->bus);
        if (EOK != ret) {
            KPrintf("PinBusInit BusRegister error %u\n", ret);
            return ret;
        }
    } else {
        KPrintf("PinBusInit BusRegister bus has been register state%u\n", pin_bus->bus.bus_state);        
    }

    return ret;
}

int PinDriverInit(struct PinDriver *pin_driver, const char *driver_name, void *data)
{
    NULL_PARAM_CHECK(pin_driver);
    NULL_PARAM_CHECK(driver_name);

    x_err_t ret = EOK;

    if (DRV_INSTALL != pin_driver->driver.driver_state) {
        pin_driver->driver.driver_type = TYPE_PIN_DRV;
        pin_driver->driver.driver_state = DRV_INSTALL;

        strncpy(pin_driver->driver.drv_name, driver_name, NAME_NUM_MAX);

        pin_driver->driver.configure = pin_driver->configure;
        pin_driver->driver.private_data = data;

        ret = PinDriverRegister(&pin_driver->driver);
        if (EOK != ret) {
            KPrintf("PinDriverInit DriverRegister error %u\n", ret);
            return ret;
        }
    } else {
        KPrintf("PinDriverInit DriverRegister driver has been register state%u\n", pin_driver->driver.driver_state);
    }

    return ret;
}

int PinReleaseBus(struct PinBus *pin_bus)
{
    NULL_PARAM_CHECK(pin_bus);

    return BusRelease(&pin_bus->bus);
}

int PinDriverAttachToBus(const char *drv_name, const char *bus_name)
{
    NULL_PARAM_CHECK(drv_name);
    NULL_PARAM_CHECK(bus_name);
    
    x_err_t ret = EOK;

    struct Bus *bus;
    struct Driver *driver;

    bus = BusFind(bus_name);
    if (NONE == bus) {
        KPrintf("PinDriverAttachToBus find spi bus error!name %s\n", bus_name);
        return ERROR;
    }

    if (TYPE_PIN_BUS == bus->bus_type) {
        driver = PinDriverFind(drv_name, TYPE_PIN_DRV);
        if (NONE == driver) {
            KPrintf("PinDriverAttachToBus find spi driver error!name %s\n", drv_name);
            return ERROR;
        }

        if (TYPE_PIN_DRV == driver->driver_type) {
            ret = DriverRegisterToBus(bus, driver);
            if (EOK != ret) {
                KPrintf("PinDriverAttachToBus DriverRegisterToBus error %u\n", ret);
                return ERROR;
            }
        }
    }

    return ret;
}

BusType PinBusInitGet(void)
{
    struct BusConfigureInfo configure_info;
    BusType pin;                                                                
    int ret = 0; 
                                                                 
    pin = BusFind(PIN_BUS_NAME);                                
    if (!pin) {                                                                         
        KPrintf("find %s failed!\n", PIN_BUS_NAME);                           
        return NONE;                                                              
    } 
                                                                          
    pin->owner_driver = BusFindDriver(pin, PIN_DRIVER_NAME);         
    pin->owner_haldev = BusFindDevice(pin, PIN_DEVICE_NAME); 

    configure_info.configure_cmd = OPE_INT;
    ret = BusDrvConfigure(pin->owner_driver, &configure_info);                                      
    if (ret != EOK) {                                                                         
        KPrintf("initialize %s failed!\n", PIN_BUS_NAME);                     
        return NONE;                                                              
    }   

    return pin;
}
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/pin/dev_pin.c`

```c
/*
* Copyright (c) 2020 AIIT XUOS Lab
* XiUOS is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*        http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
* See the Mulan PSL v2 for more details.
*/

/**
* @file dev_pin.c
* @brief register pin dev function using bus driver framework
* @version 1.0 
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#include <bus_pin.h>
#include <dev_pin.h>

static DoubleLinklistType pindev_linklist;

/*Create the Pin device linklist*/
static void PinDeviceLinkInit()
{
    InitDoubleLinkList(&pindev_linklist);
}

HardwareDevType PinDeviceFind(const char *dev_name, enum DevType DevType)
{
    NULL_PARAM_CHECK(dev_name);
    
    struct HardwareDev *device = NONE;

    DoubleLinklistType *node = NONE;
    DoubleLinklistType *head = &pindev_linklist;

    for (node = head->node_next; node != head; node = node->node_next) {
        device = SYS_DOUBLE_LINKLIST_ENTRY(node, struct HardwareDev, dev_link);
        if ((!strcmp(device->dev_name, dev_name)) && (DevType == device->dev_type)) {
            return device;
        }
    }

    KPrintf("PinDeviceFind cannot find the %s device.return NULL\n", dev_name);
    return NONE;
}

int PinDeviceRegister(struct PinHardwareDevice *pin_device, void *pin_param, const char *device_name)
{
    NULL_PARAM_CHECK(pin_device);
    NULL_PARAM_CHECK(device_name);

    x_err_t ret = EOK;    
    static x_bool dev_link_flag = RET_FALSE;

    if (!dev_link_flag) {
        PinDeviceLinkInit();
        dev_link_flag = RET_TRUE;
    }

    if (DEV_INSTALL != pin_device->haldev.dev_state) {
        strncpy(pin_device->haldev.dev_name, device_name, NAME_NUM_MAX);
        pin_device->haldev.dev_type = TYPE_PIN_DEV;
        pin_device->haldev.dev_state = DEV_INSTALL;

        pin_device->haldev.dev_done = (struct HalDevDone *)pin_device->dev_done;

        pin_device->haldev.private_data = pin_param;

        DoubleLinkListInsertNodeAfter(&pindev_linklist, &(pin_device->haldev.dev_link));
    } else {
        KPrintf("PinDeviceRegister device has been register state%u\n", pin_device->haldev.dev_state);        
    }

    return ret;
}

int PinDeviceAttachToBus(const char *dev_name, const char *bus_name)
{
    NULL_PARAM_CHECK(dev_name);
    NULL_PARAM_CHECK(bus_name);
    
    x_err_t ret = EOK;

    struct Bus *bus;
    struct HardwareDev *device;

    bus = BusFind(bus_name);
    if (NONE == bus) {
        KPrintf("PinDeviceAttachToBus find Pin bus error!name %s\n", bus_name);
        return ERROR;
    }
    
    if (TYPE_PIN_BUS == bus->bus_type) {
        device = PinDeviceFind(dev_name, TYPE_PIN_DEV);
        if (NONE == device) {
            KPrintf("PinDeviceAttachToBus find Pin device error!name %s\n", dev_name);
            return ERROR;
        }

        if (TYPE_PIN_DEV == device->dev_type) {
            ret = DeviceRegisterToBus(bus, device);
            if (EOK != ret) {
                KPrintf("PinDeviceAttachToBus DeviceRegisterToBus error %u\n", ret);
                return ERROR;
            }
        }
    }

    return EOK;
}
```
