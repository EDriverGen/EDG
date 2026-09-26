# Raw RTOS/Bus Pack: xiuos / spi

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

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/include/bus_spi.h`

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
* @file bus_spi.h
* @brief define spi bus and drv function using bus driver framework
* @version 1.0 
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#ifndef BUS_SPI_H
#define BUS_SPI_H

#include <bus.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SpiDriver
{
    struct Driver driver;

    uint32 (*configure) (void *drv, struct BusConfigureInfo *configure_info);
};

struct SpiBus
{
    struct Bus bus;

    void *private_data;
};

/*Register the spi bus*/
int SpiBusInit(struct SpiBus *spi_bus, const char *bus_name);

/*Register the spi driver*/
int SpiDriverInit(struct SpiDriver *spi_driver, const char *driver_name);

/*Release the spi device*/
int SpiReleaseBus(struct SpiBus *spi_bus);

/*Register the spi driver to the spi bus*/
int SpiDriverAttachToBus(const char *drv_name, const char *bus_name);

/*Register the driver, manage with the double linklist*/
int SpiDriverRegister(struct Driver *driver);

/*Find the register driver*/
DriverType SpiDriverFind(const char *drv_name, enum DriverType_e drv_type);

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/include/dev_spi.h`

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
* @file dev_spi.h
* @brief define spi dev function using bus driver framework
* @version 1.0 
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#ifndef DEV_SPI_H
#define DEV_SPI_H

#include <bus.h>

#ifdef __cplusplus
extern "C" {
#endif

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

struct SpiDataStandard
{
    uint8 spi_chip_select;
    uint8 spi_cs_release;

    const uint8 *tx_buff;
    uint32 tx_len;

    uint8 *rx_buff;
    uint32 rx_len;

    uint32 length;
    struct SpiDataStandard *next;
};

struct SpiMasterParam
{
    uint8 spi_work_mode;//CPOL CPHA
    uint8 spi_frame_format;//frame format
    uint8 spi_data_bit_width;//bit width
    uint8 spi_data_endian;//little endian：0，big endian：1
    uint32 spi_maxfrequency;//work frequency
};

struct SpiDmaParam 
{
    uint8 spi_master_id;
    uint8 spi_dmac_txchannel;
    uint8 spi_dmac_rxchannel;
};

struct SpiSlaveParam
{
    uint32 spi_slave_id;
    uint32 spi_cs_gpio_pin;
    uint32 spi_cs_gpio_port;
    uint32 spi_cs_select_id;
};

typedef struct
{
    struct SpiDmaParam *spi_dma_param;

    struct SpiSlaveParam *spi_slave_param;

    struct SpiMasterParam *spi_master_param;

}SpiDeviceParam;

struct SpiHardwareDevice;

struct SpiDevDone
{
    uint32 (*dev_open) (struct SpiHardwareDevice *dev);
    uint32 (*dev_close) (struct SpiHardwareDevice *dev);
    uint32 (*dev_write) (struct SpiHardwareDevice *dev, struct SpiDataStandard *msg);
    uint32 (*dev_read) (struct SpiHardwareDevice *dev, struct SpiDataStandard *msg);
};

struct SpiHardwareDevice
{
    struct HardwareDev haldev;
    SpiDeviceParam spi_param;

    x_bool spi_dev_flag;

    const struct SpiDevDone *spi_dev_done;
    
    void *private_data;
};

/*Register the spi device*/
int SpiDeviceRegister(struct SpiHardwareDevice *spi_device, void *spi_param, const char *device_name);

/*Register the spi device to the spi bus*/
int SpiDeviceAttachToBus(const char *dev_name, const char *bus_name);

/*Find the register spi device*/
HardwareDevType SpiDeviceFind(const char *dev_name, enum DevType dev_type);

/*Configure the cs pin of spi dev*/
int SpiDevConfigureCs(struct HardwareDev *dev, uint8 spi_chip_select, uint8 spi_cs_release);

#ifdef __cplusplus
}
#endif

#endif
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/spi/bus_spi.c`

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
* @file bus_spi.c
* @brief register spi bus function using bus driver framework
* @version 1.0 
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#include <bus_spi.h>
#include <dev_spi.h>

int SpiBusInit(struct SpiBus *spi_bus, const char *bus_name)
{
    NULL_PARAM_CHECK(spi_bus);
    NULL_PARAM_CHECK(bus_name);

    x_err_t ret = EOK;

    if (BUS_INSTALL != spi_bus->bus.bus_state) {
        strncpy(spi_bus->bus.bus_name, bus_name, NAME_NUM_MAX);

        spi_bus->bus.bus_type = TYPE_SPI_BUS;
        spi_bus->bus.bus_state = BUS_INSTALL;
        spi_bus->bus.private_data = spi_bus->private_data;

        ret = BusRegister(&spi_bus->bus);
        if (EOK != ret) {
            KPrintf("SpiBusInit BusRegister error %u\n", ret);
            return ret;
        }
    } else {
        KPrintf("SpiBusInit BusRegister bus has been register state%u\n", spi_bus->bus.bus_state);        
    }

    return ret;
}

int SpiDriverInit(struct SpiDriver *spi_driver, const char *driver_name)
{
    NULL_PARAM_CHECK(spi_driver);
    NULL_PARAM_CHECK(driver_name);

    x_err_t ret = EOK;

    if (DRV_INSTALL != spi_driver->driver.driver_state) {
        spi_driver->driver.driver_type = TYPE_SPI_DRV;
        spi_driver->driver.driver_state = DRV_INSTALL;

        strncpy(spi_driver->driver.drv_name, driver_name, NAME_NUM_MAX);

        spi_driver->driver.configure = spi_driver->configure;

        ret = SpiDriverRegister(&spi_driver->driver);
        if (EOK != ret) {
            KPrintf("SpiDriverInit DriverRegister error %u\n", ret);
            return ret;
        }
    } else {
        KPrintf("SpiDriverInit DriverRegister driver has been register state%u\n", spi_driver->driver.driver_state);
    }

    return ret;
}

int SpiReleaseBus(struct SpiBus *spi_bus)
{
    NULL_PARAM_CHECK(spi_bus);

    return BusRelease(&spi_bus->bus);
}

int SpiDriverAttachToBus(const char *drv_name, const char *bus_name)
{
    NULL_PARAM_CHECK(drv_name);
    NULL_PARAM_CHECK(bus_name);
    
    x_err_t ret = EOK;

    struct Bus *bus;
    struct Driver *driver;

    bus = BusFind(bus_name);
    if (NONE == bus) {
        KPrintf("SpiDriverAttachToBus find spi bus error!name %s\n", bus_name);
        return ERROR;
    }

    if (TYPE_SPI_BUS == bus->bus_type) {
        driver = SpiDriverFind(drv_name, TYPE_SPI_DRV);
        if (NONE == driver) {
            KPrintf("SpiDriverAttachToBus find spi driver error!name %s\n", drv_name);
            return ERROR;
        }

        if (TYPE_SPI_DRV == driver->driver_type) {
            ret = DriverRegisterToBus(bus, driver);
            if (EOK != ret) {
                KPrintf("SpiDriverAttachToBus DriverRegisterToBus error %u\n", ret);
                return ERROR;
            }
        }
    }

    return ret;
}
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/resources/spi/dev_spi.c`

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
* @file dev_spi.c
* @brief register spi dev function using bus driver framework
* @version 1.0
* @author AIIT XUOS Lab
* @date 2021-04-24
*/

#include <bus_spi.h>
#include <dev_spi.h>

static DoubleLinklistType spidev_linklist;

/*Create the spi device linklist*/
static void SpiDeviceLinkInit()
{
    InitDoubleLinkList(&spidev_linklist);
}

static uint32 SpiDeviceOpen(void *dev)
{
    NULL_PARAM_CHECK(dev);

    SpiDevConfigureCs(dev, 1, 0);

    return EOK;
}

static uint32 SpiDeviceClose(void *dev)
{
    NULL_PARAM_CHECK(dev);

    SpiDevConfigureCs(dev, 0, 1);

    return EOK;
}

static uint32 SpiDeviceWrite(void *dev, struct BusBlockWriteParam *write_param)
{
    NULL_PARAM_CHECK(dev);
    NULL_PARAM_CHECK(write_param);

    int ret;
    struct SpiHardwareDevice *spi_dev = (struct SpiHardwareDevice *)dev;
    struct SpiDataStandard *spi_msg;

    spi_msg = (struct SpiDataStandard *)x_malloc(sizeof(struct SpiDataStandard));
    if (NONE == spi_msg) {
        KPrintf("SpiDeviceWrite x_malloc msg error\n");
        x_free(spi_msg);
        return ERROR;
    }

    //memset(spi_msg, 0, sizeof(struct SpiDataStandard));

    spi_msg->tx_buff = (uint8 *)write_param->buffer;
    spi_msg->rx_buff = NONE;
    spi_msg->length = write_param->size;
    spi_msg->spi_chip_select = 0;
    spi_msg->spi_cs_release = 0;
    spi_msg->next = NONE;

    ret = spi_dev->spi_dev_done->dev_write(spi_dev, spi_msg);
    x_free(spi_msg);

    return ret;
}

static uint32 SpiDeviceRead(void *dev, struct BusBlockReadParam *read_param)
{
    NULL_PARAM_CHECK(dev);
    NULL_PARAM_CHECK(read_param);

    int ret;
    struct SpiHardwareDevice *spi_dev = (struct SpiHardwareDevice *)dev;
    struct SpiDataStandard *spi_msg;

    spi_msg = (struct SpiDataStandard *)x_malloc(sizeof(struct SpiDataStandard));
    if (NONE == spi_msg) {
        KPrintf("SpiDeviceRead x_malloc msg error\n");
        x_free(spi_msg);
        return ERROR;
    }

    //memset(spi_msg, 0, sizeof(struct SpiDataStandard));

    spi_msg->tx_buff = NONE;
    spi_msg->rx_buff = (uint8 *)read_param->buffer;
    spi_msg->length = read_param->size;
    spi_msg->spi_chip_select = 0;
    spi_msg->spi_cs_release = 0;
    spi_msg->next = NONE;

    ret = spi_dev->spi_dev_done->dev_read(spi_dev, spi_msg);
    x_free(spi_msg);

    return ret;
}

static const struct HalDevDone dev_done =
{
    .open = SpiDeviceOpen,
    .close = SpiDeviceClose,
    .write = SpiDeviceWrite,
    .read = SpiDeviceRead,
};

HardwareDevType SpiDeviceFind(const char *dev_name, enum DevType dev_type)
{
    NULL_PARAM_CHECK(dev_name);

    struct HardwareDev *device = NONE;

    DoubleLinklistType *node = NONE;
    DoubleLinklistType *head = &spidev_linklist;

    for (node = head->node_next; node != head; node = node->node_next) {
        device = SYS_DOUBLE_LINKLIST_ENTRY(node, struct HardwareDev, dev_link);
        if ((!strcmp(device->dev_name, dev_name)) && (dev_type == device->dev_type)) {
            return device;
        }
    }

    KPrintf("SpiDeviceFind cannot find the %s device.return NULL\n", dev_name);
    return NONE;
}

int SpiDeviceRegister(struct SpiHardwareDevice *spi_device, void *spi_param, const char *device_name)
{
    NULL_PARAM_CHECK(spi_device);
    NULL_PARAM_CHECK(device_name);

    x_err_t ret = EOK;
    static x_bool dev_link_flag = RET_FALSE;

    if (!dev_link_flag) {
        SpiDeviceLinkInit();
        dev_link_flag = RET_TRUE;
    }

    if (DEV_INSTALL != spi_device->haldev.dev_state) {
        strncpy(spi_device->haldev.dev_name, device_name, NAME_NUM_MAX);
        spi_device->haldev.dev_type = TYPE_SPI_DEV;
        spi_device->haldev.dev_state = DEV_INSTALL;

        //only spi bus dev need to register dev_done
        if (RET_TRUE != spi_device->spi_dev_flag) {
            spi_device->haldev.dev_done = &dev_done;
        }

        spi_device->haldev.private_data = spi_param;

        DoubleLinkListInsertNodeAfter(&spidev_linklist, &(spi_device->haldev.dev_link));
    } else {
        KPrintf("SpiDeviceRegister device has been register state%u\n", spi_device->haldev.dev_state);
    }

    return ret;
}

int SpiDeviceAttachToBus(const char *dev_name, const char *bus_name)
{
    NULL_PARAM_CHECK(dev_name);
    NULL_PARAM_CHECK(bus_name);

    x_err_t ret = EOK;

    struct Bus *bus;
    struct HardwareDev *device;

    bus = BusFind(bus_name);
    if (NONE == bus) {
        KPrintf("SpiDeviceAttachToBus find spi bus error!name %s\n", bus_name);
        return ERROR;
    }

    if (TYPE_SPI_BUS == bus->bus_type) {
        device = SpiDeviceFind(dev_name, TYPE_SPI_DEV);
        if (NONE == device) {
            KPrintf("SpiDeviceAttachToBus find spi device error!name %s\n", dev_name);
            return ERROR;
        }

        if (TYPE_SPI_DEV == device->dev_type) {
            ret = DeviceRegisterToBus(bus, device);
            if (EOK != ret) {
                KPrintf("SpiDeviceAttachToBus DeviceRegisterToBus error %u\n", ret);
                return ERROR;
            }
        }
    }

    return EOK;
}

int SpiDevConfigureCs(struct HardwareDev *dev, uint8 spi_chip_select, uint8 spi_cs_release)
{
    NULL_PARAM_CHECK(dev);

    int ret;
    struct SpiHardwareDevice *spi_dev = (struct SpiHardwareDevice *)dev;
    struct SpiDataStandard *msg;

    msg = (struct SpiDataStandard *)x_malloc(sizeof(struct SpiDataStandard));
    if (NONE == msg) {
        KPrintf("SpiDevConfigureCs x_malloc msg error\n");
        x_free(msg);
        return ERROR;
    }

    //memset(msg, 0, sizeof(struct SpiDataStandard));
    msg->length = 0;
    msg->rx_buff = NONE;
    msg->tx_buff = NONE;
    msg->next = NONE;
    msg->spi_chip_select = spi_chip_select;
    msg->spi_cs_release = spi_cs_release;

    ret = spi_dev->spi_dev_done->dev_write(spi_dev, msg);

    x_free(msg);
    return ret;
}
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/stm32f1xx_hal_def.h`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_def.h
  * @author  MCD Application Team
  * @brief   This file contains HAL common defines, enumeration, macros and
  *          structures definitions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1xx_HAL_DEF
#define __STM32F1xx_HAL_DEF

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stm32f1xx.h>
#if defined(USE_HAL_LEGACY)
#include "Legacy/stm32_hal_legacy.h"
#endif
#include <stddef.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  HAL Status structures definition
  */
typedef enum
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/**
  * @brief  HAL Lock structures definition
  */
typedef enum
{
  HAL_UNLOCKED = 0x00U,
  HAL_LOCKED   = 0x01U
} HAL_LockTypeDef;

/* Exported macro ------------------------------------------------------------*/
#define HAL_MAX_DELAY      0xFFFFFFFFU

#define HAL_IS_BIT_SET(REG, BIT)         (((REG) & (BIT)) != 0U)
#define HAL_IS_BIT_CLR(REG, BIT)         (((REG) & (BIT)) == 0U)

#define __HAL_LINKDMA(__HANDLE__, __PPP_DMA_FIELD__, __DMA_HANDLE__)               \
                        do{                                                      \
                              (__HANDLE__)->__PPP_DMA_FIELD__ = &(__DMA_HANDLE__); \
                              (__DMA_HANDLE__).Parent = (__HANDLE__);             \
                          } while(0U)

#define UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */

/** @brief Reset the Handle's State field.
  * @param __HANDLE__ specifies the Peripheral Handle.
  * @note  This macro can be used for the following purpose:
  *          - When the Handle is declared as local variable; before passing it as parameter
  *            to HAL_PPP_Init() for the first time, it is mandatory to use this macro
  *            to set to 0 the Handle's "State" field.
  *            Otherwise, "State" field may have any random value and the first time the function
  *            HAL_PPP_Init() is called, the low level hardware initialization will be missed
  *            (i.e. HAL_PPP_MspInit() will not be executed).
  *          - When there is a need to reconfigure the low level hardware: instead of calling
  *            HAL_PPP_DeInit() then HAL_PPP_Init(), user can make a call to this macro then HAL_PPP_Init().
  *            In this later function, when the Handle's "State" field is set to 0, it will execute the function
  *            HAL_PPP_MspInit() which will reconfigure the low level hardware.
  * @retval None
  */
#define __HAL_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = 0U)

#if (USE_RTOS == 1U)
/* Reserved for future use */
#error "USE_RTOS should be 0 in the current HAL release"
#else
#define __HAL_LOCK(__HANDLE__)                                           \
                                do{                                        \
                                    if((__HANDLE__)->Lock == HAL_LOCKED)   \
                                    {                                      \
                                       return HAL_BUSY;                    \
                                    }                                      \
                                    else                                   \
                                    {                                      \
                                       (__HANDLE__)->Lock = HAL_LOCKED;    \
                                    }                                      \
                                  }while (0U)

#define __HAL_UNLOCK(__HANDLE__)                                          \
                                  do{                                       \
                                      (__HANDLE__)->Lock = HAL_UNLOCKED;    \
                                    }while (0U)
#endif /* USE_RTOS */

#if defined ( __GNUC__ ) && !defined (__CC_ARM) /* GNU Compiler */
#ifndef __weak
#define __weak   __attribute__((weak))
#endif /* __weak */
#ifndef __packed
#define __packed __attribute__((__packed__))
#endif /* __packed */
#endif /* __GNUC__ */


/* Macro to get variable aligned on 4-bytes, for __ICCARM__ the directive "#pragma data_alignment=4" must be used instead */
#if defined ( __GNUC__ ) && !defined (__CC_ARM) /* GNU Compiler */
#ifndef __ALIGN_END
#define __ALIGN_END    __attribute__ ((aligned (4)))
#endif /* __ALIGN_END */
#ifndef __ALIGN_BEGIN
#define __ALIGN_BEGIN
#endif /* __ALIGN_BEGIN */
#else
#ifndef __ALIGN_END
#define __ALIGN_END
#endif /* __ALIGN_END */
#ifndef __ALIGN_BEGIN
#if defined   (__CC_ARM)      /* ARM Compiler */
#define __ALIGN_BEGIN    __align(4)
#elif defined (__ICCARM__)    /* IAR Compiler */
#define __ALIGN_BEGIN
#endif /* __CC_ARM */
#endif /* __ALIGN_BEGIN */
#endif /* __GNUC__ */


/**
  * @brief  __RAM_FUNC definition
  */
#if defined ( __CC_ARM   )
/* ARM Compiler
   ------------
   RAM functions are defined using the toolchain options.
   Functions that are executed in RAM should reside in a separate source module.
   Using the 'Options for File' dialog you can simply change the 'Code / Const'
   area of a module to a memory space in physical RAM.
   Available memory areas are declared in the 'Target' tab of the 'Options for Target'
   dialog.
*/
#define __RAM_FUNC

#elif defined ( __ICCARM__ )
/* ICCARM Compiler
   ---------------
   RAM functions are defined using a specific toolchain keyword "__ramfunc".
*/
#define __RAM_FUNC __ramfunc

#elif defined   (  __GNUC__  )
/* GNU Compiler
   ------------
  RAM functions are defined using a specific toolchain attribute
   "__attribute__((section(".RamFunc")))".
*/
#define __RAM_FUNC __attribute__((section(".RamFunc")))

#endif

/**
  * @brief  __NOINLINE definition
  */
#if defined ( __CC_ARM   ) || defined   (  __GNUC__  )
/* ARM & GNUCompiler
   ----------------
*/
#define __NOINLINE __attribute__ ( (noinline) )

#elif defined ( __ICCARM__ )
/* ICCARM Compiler
   ---------------
*/
#define __NOINLINE _Pragma("optimize = no_inline")

#endif

#ifdef __cplusplus
}
#endif

#endif /* ___STM32F1xx_HAL_DEF */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/stm32f1xx_hal_spi.h`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_spi.h
  * @author  MCD Application Team
  * @brief   Header file of SPI HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32F1xx_HAL_SPI_H
#define STM32F1xx_HAL_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal_def.h"

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @addtogroup SPI
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup SPI_Exported_Types SPI Exported Types
  * @{
  */

/**
  * @brief  SPI Configuration Structure definition
  */
typedef struct
{
  uint32_t Mode;                /*!< Specifies the SPI operating mode.
                                     This parameter can be a value of @ref SPI_Mode */

  uint32_t Direction;           /*!< Specifies the SPI bidirectional mode state.
                                     This parameter can be a value of @ref SPI_Direction */

  uint32_t DataSize;            /*!< Specifies the SPI data size.
                                     This parameter can be a value of @ref SPI_Data_Size */

  uint32_t CLKPolarity;         /*!< Specifies the serial clock steady state.
                                     This parameter can be a value of @ref SPI_Clock_Polarity */

  uint32_t CLKPhase;            /*!< Specifies the clock active edge for the bit capture.
                                     This parameter can be a value of @ref SPI_Clock_Phase */

  uint32_t NSS;                 /*!< Specifies whether the NSS signal is managed by
                                     hardware (NSS pin) or by software using the SSI bit.
                                     This parameter can be a value of @ref SPI_Slave_Select_management */

  uint32_t BaudRatePrescaler;   /*!< Specifies the Baud Rate prescaler value which will be
                                     used to configure the transmit and receive SCK clock.
                                     This parameter can be a value of @ref SPI_BaudRate_Prescaler
                                     @note The communication clock is derived from the master
                                     clock. The slave clock does not need to be set. */

  uint32_t FirstBit;            /*!< Specifies whether data transfers start from MSB or LSB bit.
                                     This parameter can be a value of @ref SPI_MSB_LSB_transmission */

  uint32_t TIMode;              /*!< Specifies if the TI mode is enabled or not.
                                     This parameter can be a value of @ref SPI_TI_mode */

  uint32_t CRCCalculation;      /*!< Specifies if the CRC calculation is enabled or not.
                                     This parameter can be a value of @ref SPI_CRC_Calculation */

  uint32_t CRCPolynomial;       /*!< Specifies the polynomial used for the CRC calculation.
                                     This parameter must be an odd number between Min_Data = 1 and Max_Data = 65535 */
} SPI_InitTypeDef;

/**
  * @brief  HAL SPI State structure definition
  */
typedef enum
{
  HAL_SPI_STATE_RESET      = 0x00U,    /*!< Peripheral not Initialized                         */
  HAL_SPI_STATE_READY      = 0x01U,    /*!< Peripheral Initialized and ready for use           */
  HAL_SPI_STATE_BUSY       = 0x02U,    /*!< an internal process is ongoing                     */
  HAL_SPI_STATE_BUSY_TX    = 0x03U,    /*!< Data Transmission process is ongoing               */
  HAL_SPI_STATE_BUSY_RX    = 0x04U,    /*!< Data Reception process is ongoing                  */
  HAL_SPI_STATE_BUSY_TX_RX = 0x05U,    /*!< Data Transmission and Reception process is ongoing */
  HAL_SPI_STATE_ERROR      = 0x06U,    /*!< SPI error state                                    */
  HAL_SPI_STATE_ABORT      = 0x07U     /*!< SPI abort is ongoing                               */
} HAL_SPI_StateTypeDef;

/**
  * @brief  SPI handle Structure definition
  */
typedef struct __SPI_HandleTypeDef
{
  SPI_TypeDef                *Instance;      /*!< SPI registers base address               */

  SPI_InitTypeDef            Init;           /*!< SPI communication parameters             */

  uint8_t                    *pTxBuffPtr;    /*!< Pointer to SPI Tx transfer Buffer        */

  uint16_t                   TxXferSize;     /*!< SPI Tx Transfer size                     */

  __IO uint16_t              TxXferCount;    /*!< SPI Tx Transfer Counter                  */

  uint8_t                    *pRxBuffPtr;    /*!< Pointer to SPI Rx transfer Buffer        */

  uint16_t                   RxXferSize;     /*!< SPI Rx Transfer size                     */

  __IO uint16_t              RxXferCount;    /*!< SPI Rx Transfer Counter                  */

  void (*RxISR)(struct __SPI_HandleTypeDef *hspi);   /*!< function pointer on Rx ISR       */

  void (*TxISR)(struct __SPI_HandleTypeDef *hspi);   /*!< function pointer on Tx ISR       */

  DMA_HandleTypeDef          *hdmatx;        /*!< SPI Tx DMA Handle parameters             */

  DMA_HandleTypeDef          *hdmarx;        /*!< SPI Rx DMA Handle parameters             */

  HAL_LockTypeDef            Lock;           /*!< Locking object                           */

  __IO HAL_SPI_StateTypeDef  State;          /*!< SPI communication state                  */

  __IO uint32_t              ErrorCode;      /*!< SPI Error code                           */

#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
  void (* TxCpltCallback)(struct __SPI_HandleTypeDef *hspi);             /*!< SPI Tx Completed callback          */
  void (* RxCpltCallback)(struct __SPI_HandleTypeDef *hspi);             /*!< SPI Rx Completed callback          */
  void (* TxRxCpltCallback)(struct __SPI_HandleTypeDef *hspi);           /*!< SPI TxRx Completed callback        */
  void (* TxHalfCpltCallback)(struct __SPI_HandleTypeDef *hspi);         /*!< SPI Tx Half Completed callback     */
  void (* RxHalfCpltCallback)(struct __SPI_HandleTypeDef *hspi);         /*!< SPI Rx Half Completed callback     */
  void (* TxRxHalfCpltCallback)(struct __SPI_HandleTypeDef *hspi);       /*!< SPI TxRx Half Completed callback   */
  void (* ErrorCallback)(struct __SPI_HandleTypeDef *hspi);              /*!< SPI Error callback                 */
  void (* AbortCpltCallback)(struct __SPI_HandleTypeDef *hspi);          /*!< SPI Abort callback                 */
  void (* MspInitCallback)(struct __SPI_HandleTypeDef *hspi);            /*!< SPI Msp Init callback              */
  void (* MspDeInitCallback)(struct __SPI_HandleTypeDef *hspi);          /*!< SPI Msp DeInit callback            */

#endif  /* USE_HAL_SPI_REGISTER_CALLBACKS */
} SPI_HandleTypeDef;

#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
/**
  * @brief  HAL SPI Callback ID enumeration definition
  */
typedef enum
{
  HAL_SPI_TX_COMPLETE_CB_ID             = 0x00U,    /*!< SPI Tx Completed callback ID         */
  HAL_SPI_RX_COMPLETE_CB_ID             = 0x01U,    /*!< SPI Rx Completed callback ID         */
  HAL_SPI_TX_RX_COMPLETE_CB_ID          = 0x02U,    /*!< SPI TxRx Completed callback ID       */
  HAL_SPI_TX_HALF_COMPLETE_CB_ID        = 0x03U,    /*!< SPI Tx Half Completed callback ID    */
  HAL_SPI_RX_HALF_COMPLETE_CB_ID        = 0x04U,    /*!< SPI Rx Half Completed callback ID    */
  HAL_SPI_TX_RX_HALF_COMPLETE_CB_ID     = 0x05U,    /*!< SPI TxRx Half Completed callback ID  */
  HAL_SPI_ERROR_CB_ID                   = 0x06U,    /*!< SPI Error callback ID                */
  HAL_SPI_ABORT_CB_ID                   = 0x07U,    /*!< SPI Abort callback ID                */
  HAL_SPI_MSPINIT_CB_ID                 = 0x08U,    /*!< SPI Msp Init callback ID             */
  HAL_SPI_MSPDEINIT_CB_ID               = 0x09U     /*!< SPI Msp DeInit callback ID           */

} HAL_SPI_CallbackIDTypeDef;

/**
  * @brief  HAL SPI Callback pointer definition
  */
typedef  void (*pSPI_CallbackTypeDef)(SPI_HandleTypeDef *hspi); /*!< pointer to an SPI callback function */

#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup SPI_Exported_Constants SPI Exported Constants
  * @{
  */

/** @defgroup SPI_Error_Code SPI Error Code
  * @{
  */
#define HAL_SPI_ERROR_NONE              (0x00000000U)   /*!< No error                               */
#define HAL_SPI_ERROR_MODF              (0x00000001U)   /*!< MODF error                             */
#define HAL_SPI_ERROR_CRC               (0x00000002U)   /*!< CRC error                              */
#define HAL_SPI_ERROR_OVR               (0x00000004U)   /*!< OVR error                              */
#define HAL_SPI_ERROR_DMA               (0x00000010U)   /*!< DMA transfer error                     */
#define HAL_SPI_ERROR_FLAG              (0x00000020U)   /*!< Error on RXNE/TXE/BSY Flag             */
#define HAL_SPI_ERROR_ABORT             (0x00000040U)   /*!< Error during SPI Abort procedure       */
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
#define HAL_SPI_ERROR_INVALID_CALLBACK  (0x00000080U)   /*!< Invalid Callback error                 */
#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
/**
  * @}
  */

/** @defgroup SPI_Mode SPI Mode
  * @{
  */
#define SPI_MODE_SLAVE                  (0x00000000U)
#define SPI_MODE_MASTER                 (SPI_CR1_MSTR | SPI_CR1_SSI)
/**
  * @}
  */

/** @defgroup SPI_Direction SPI Direction Mode
  * @{
  */
#define SPI_DIRECTION_2LINES            (0x00000000U)
#define SPI_DIRECTION_2LINES_RXONLY     SPI_CR1_RXONLY
#define SPI_DIRECTION_1LINE             SPI_CR1_BIDIMODE
/**
  * @}
  */

/** @defgroup SPI_Data_Size SPI Data Size
  * @{
  */
#define SPI_DATASIZE_8BIT               (0x00000000U)
#define SPI_DATASIZE_16BIT              SPI_CR1_DFF
/**
  * @}
  */

/** @defgroup SPI_Clock_Polarity SPI Clock Polarity
  * @{
  */
#define SPI_POLARITY_LOW                (0x00000000U)
#define SPI_POLARITY_HIGH               SPI_CR1_CPOL
/**
  * @}
  */

/** @defgroup SPI_Clock_Phase SPI Clock Phase
  * @{
  */
#define SPI_PHASE_1EDGE                 (0x00000000U)
#define SPI_PHASE_2EDGE                 SPI_CR1_CPHA
/**
  * @}
  */

/** @defgroup SPI_Slave_Select_management SPI Slave Select Management
  * @{
  */
#define SPI_NSS_SOFT                    SPI_CR1_SSM
#define SPI_NSS_HARD_INPUT              (0x00000000U)
#define SPI_NSS_HARD_OUTPUT             (SPI_CR2_SSOE << 16U)
/**
  * @}
  */

/** @defgroup SPI_BaudRate_Prescaler SPI BaudRate Prescaler
  * @{
  */
#define SPI_BAUDRATEPRESCALER_2         (0x00000000U)
#define SPI_BAUDRATEPRESCALER_4         (SPI_CR1_BR_0)
#define SPI_BAUDRATEPRESCALER_8         (SPI_CR1_BR_1)
#define SPI_BAUDRATEPRESCALER_16        (SPI_CR1_BR_1 | SPI_CR1_BR_0)
#define SPI_BAUDRATEPRESCALER_32        (SPI_CR1_BR_2)
#define SPI_BAUDRATEPRESCALER_64        (SPI_CR1_BR_2 | SPI_CR1_BR_0)
#define SPI_BAUDRATEPRESCALER_128       (SPI_CR1_BR_2 | SPI_CR1_BR_1)
#define SPI_BAUDRATEPRESCALER_256       (SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0)
/**
  * @}
  */

/** @defgroup SPI_MSB_LSB_transmission SPI MSB LSB Transmission
  * @{
  */
#define SPI_FIRSTBIT_MSB                (0x00000000U)
#define SPI_FIRSTBIT_LSB                SPI_CR1_LSBFIRST
/**
  * @}
  */

/** @defgroup SPI_TI_mode SPI TI Mode
  * @{
  */
#define SPI_TIMODE_DISABLE              (0x00000000U)
/**
  * @}
  */

/** @defgroup SPI_CRC_Calculation SPI CRC Calculation
  * @{
  */
#define SPI_CRCCALCULATION_DISABLE      (0x00000000U)
#define SPI_CRCCALCULATION_ENABLE       SPI_CR1_CRCEN
/**
  * @}
  */

/** @defgroup SPI_Interrupt_definition SPI Interrupt Definition
  * @{
  */
#define SPI_IT_TXE                      SPI_CR2_TXEIE
#define SPI_IT_RXNE                     SPI_CR2_RXNEIE
#define SPI_IT_ERR                      SPI_CR2_ERRIE
/**
  * @}
  */

/** @defgroup SPI_Flags_definition SPI Flags Definition
  * @{
  */
#define SPI_FLAG_RXNE                   SPI_SR_RXNE   /* SPI status flag: Rx buffer not empty flag       */
#define SPI_FLAG_TXE                    SPI_SR_TXE    /* SPI status flag: Tx buffer empty flag           */
#define SPI_FLAG_BSY                    SPI_SR_BSY    /* SPI status flag: Busy flag                      */
#define SPI_FLAG_CRCERR                 SPI_SR_CRCERR /* SPI Error flag: CRC error flag                  */
#define SPI_FLAG_MODF                   SPI_SR_MODF   /* SPI Error flag: Mode fault flag                 */
#define SPI_FLAG_OVR                    SPI_SR_OVR    /* SPI Error flag: Overrun flag                    */
#define SPI_FLAG_MASK                   (SPI_SR_RXNE | SPI_SR_TXE | SPI_SR_BSY\
                                         | SPI_SR_CRCERR | SPI_SR_MODF | SPI_SR_OVR)
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup SPI_Exported_Macros SPI Exported Macros
  * @{
  */

/** @brief  Reset SPI handle state.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
#define __HAL_SPI_RESET_HANDLE_STATE(__HANDLE__)                do{                                                  \
                                                                    (__HANDLE__)->State = HAL_SPI_STATE_RESET;       \
                                                                    (__HANDLE__)->MspInitCallback = NULL;            \
                                                                    (__HANDLE__)->MspDeInitCallback = NULL;          \
                                                                  } while(0)
#else
#define __HAL_SPI_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_SPI_STATE_RESET)
#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */

/** @brief  Enable the specified SPI interrupts.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @param  __INTERRUPT__ specifies the interrupt source to enable.
  *         This parameter can be one of the following values:
  *            @arg SPI_IT_TXE: Tx buffer empty interrupt enable
  *            @arg SPI_IT_RXNE: RX buffer not empty interrupt enable
  *            @arg SPI_IT_ERR: Error interrupt enable
  * @retval None
  */
#define __HAL_SPI_ENABLE_IT(__HANDLE__, __INTERRUPT__)   SET_BIT((__HANDLE__)->Instance->CR2, (__INTERRUPT__))

/** @brief  Disable the specified SPI interrupts.
  * @param  __HANDLE__ specifies the SPI handle.
  *         This parameter can be SPIx where x: 1, 2, or 3 to select the SPI peripheral.
  * @param  __INTERRUPT__ specifies the interrupt source to disable.
  *         This parameter can be one of the following values:
  *            @arg SPI_IT_TXE: Tx buffer empty interrupt enable
  *            @arg SPI_IT_RXNE: RX buffer not empty interrupt enable
  *            @arg SPI_IT_ERR: Error interrupt enable
  * @retval None
  */
#define __HAL_SPI_DISABLE_IT(__HANDLE__, __INTERRUPT__)  CLEAR_BIT((__HANDLE__)->Instance->CR2, (__INTERRUPT__))

/** @brief  Check whether the specified SPI interrupt source is enabled or not.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @param  __INTERRUPT__ specifies the SPI interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg SPI_IT_TXE: Tx buffer empty interrupt enable
  *            @arg SPI_IT_RXNE: RX buffer not empty interrupt enable
  *            @arg SPI_IT_ERR: Error interrupt enable
  * @retval The new state of __IT__ (TRUE or FALSE).
  */
#define __HAL_SPI_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__) ((((__HANDLE__)->Instance->CR2\
                                                              & (__INTERRUPT__)) == (__INTERRUPT__)) ? SET : RESET)

/** @brief  Check whether the specified SPI flag is set or not.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @param  __FLAG__ specifies the flag to check.
  *         This parameter can be one of the following values:
  *            @arg SPI_FLAG_RXNE: Receive buffer not empty flag
  *            @arg SPI_FLAG_TXE: Transmit buffer empty flag
  *            @arg SPI_FLAG_CRCERR: CRC error flag
  *            @arg SPI_FLAG_MODF: Mode fault flag
  *            @arg SPI_FLAG_OVR: Overrun flag
  *            @arg SPI_FLAG_BSY: Busy flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_SPI_GET_FLAG(__HANDLE__, __FLAG__) ((((__HANDLE__)->Instance->SR) & (__FLAG__)) == (__FLAG__))

/** @brief  Clear the SPI CRCERR pending flag.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_CLEAR_CRCERRFLAG(__HANDLE__) ((__HANDLE__)->Instance->SR = (uint16_t)(~SPI_FLAG_CRCERR))

/** @brief  Clear the SPI MODF pending flag.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_CLEAR_MODFFLAG(__HANDLE__)             \
  do{                                                    \
    __IO uint32_t tmpreg_modf = 0x00U;                   \
    tmpreg_modf = (__HANDLE__)->Instance->SR;            \
    CLEAR_BIT((__HANDLE__)->Instance->CR1, SPI_CR1_SPE); \
    UNUSED(tmpreg_modf);                                 \
  } while(0U)

/** @brief  Clear the SPI OVR pending flag.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_CLEAR_OVRFLAG(__HANDLE__)        \
  do{                                              \
    __IO uint32_t tmpreg_ovr = 0x00U;              \
    tmpreg_ovr = (__HANDLE__)->Instance->DR;       \
    tmpreg_ovr = (__HANDLE__)->Instance->SR;       \
    UNUSED(tmpreg_ovr);                            \
  } while(0U)

/** @brief  Enable the SPI peripheral.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_ENABLE(__HANDLE__)  SET_BIT((__HANDLE__)->Instance->CR1, SPI_CR1_SPE)

/** @brief  Disable the SPI peripheral.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define __HAL_SPI_DISABLE(__HANDLE__) CLEAR_BIT((__HANDLE__)->Instance->CR1, SPI_CR1_SPE)

/**
  * @}
  */

/* Private constants ---------------------------------------------------------*/
/** @defgroup SPI_Private_Constants SPI Private Constants
  * @{
  */
#define SPI_INVALID_CRC_ERROR     0U          /* CRC error wrongly detected */
#define SPI_VALID_CRC_ERROR       1U          /* CRC error is true */
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup SPI_Private_Macros SPI Private Macros
  * @{
  */

/** @brief  Set the SPI transmit-only mode.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define SPI_1LINE_TX(__HANDLE__)  SET_BIT((__HANDLE__)->Instance->CR1, SPI_CR1_BIDIOE)

/** @brief  Set the SPI receive-only mode.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define SPI_1LINE_RX(__HANDLE__)  CLEAR_BIT((__HANDLE__)->Instance->CR1, SPI_CR1_BIDIOE)

/** @brief  Reset the CRC calculation of the SPI.
  * @param  __HANDLE__ specifies the SPI Handle.
  *         This parameter can be SPI where x: 1, 2, or 3 to select the SPI peripheral.
  * @retval None
  */
#define SPI_RESET_CRC(__HANDLE__) do{CLEAR_BIT((__HANDLE__)->Instance->CR1, SPI_CR1_CRCEN);\
                                       SET_BIT((__HANDLE__)->Instance->CR1, SPI_CR1_CRCEN);}while(0U)

/** @brief  Check whether the specified SPI flag is set or not.
  * @param  __SR__  copy of SPI SR regsiter.
  * @param  __FLAG__ specifies the flag to check.
  *         This parameter can be one of the following values:
  *            @arg SPI_FLAG_RXNE: Receive buffer not empty flag
  *            @arg SPI_FLAG_TXE: Transmit buffer empty flag
  *            @arg SPI_FLAG_CRCERR: CRC error flag
  *            @arg SPI_FLAG_MODF: Mode fault flag
  *            @arg SPI_FLAG_OVR: Overrun flag
  *            @arg SPI_FLAG_BSY: Busy flag
  * @retval SET or RESET.
  */
#define SPI_CHECK_FLAG(__SR__, __FLAG__)         ((((__SR__) & ((__FLAG__) & SPI_FLAG_MASK)) == ((__FLAG__) & SPI_FLAG_MASK)) ? SET : RESET)

/** @brief  Check whether the specified SPI Interrupt is set or not.
  * @param  __CR2__  copy of SPI CR2 regsiter.
  * @param  __INTERRUPT__ specifies the SPI interrupt source to check.
  *         This parameter can be one of the following values:
  *            @arg SPI_IT_TXE: Tx buffer empty interrupt enable
  *            @arg SPI_IT_RXNE: RX buffer not empty interrupt enable
  *            @arg SPI_IT_ERR: Error interrupt enable
  * @retval SET or RESET.
  */
#define SPI_CHECK_IT_SOURCE(__CR2__, __INTERRUPT__)      ((((__CR2__) & (__INTERRUPT__)) == (__INTERRUPT__)) ? SET : RESET)

/** @brief  Checks if SPI Mode parameter is in allowed range.
  * @param  __MODE__ specifies the SPI Mode.
  *         This parameter can be a value of @ref SPI_Mode
  * @retval None
  */
#define IS_SPI_MODE(__MODE__) (((__MODE__) == SPI_MODE_SLAVE) || \
                               ((__MODE__) == SPI_MODE_MASTER))

/** @brief  Checks if SPI Direction Mode parameter is in allowed range.
  * @param  __MODE__ specifies the SPI Direction Mode.
  *         This parameter can be a value of @ref SPI_Direction
  * @retval None
  */
#define IS_SPI_DIRECTION(__MODE__) (((__MODE__) == SPI_DIRECTION_2LINES)        || \
                                    ((__MODE__) == SPI_DIRECTION_2LINES_RXONLY) || \
                                    ((__MODE__) == SPI_DIRECTION_1LINE))

/** @brief  Checks if SPI Direction Mode parameter is 2 lines.
  * @param  __MODE__ specifies the SPI Direction Mode.
  * @retval None
  */
#define IS_SPI_DIRECTION_2LINES(__MODE__) ((__MODE__) == SPI_DIRECTION_2LINES)

/** @brief  Checks if SPI Direction Mode parameter is 1 or 2 lines.
  * @param  __MODE__ specifies the SPI Direction Mode.
  * @retval None
  */
#define IS_SPI_DIRECTION_2LINES_OR_1LINE(__MODE__) (((__MODE__) == SPI_DIRECTION_2LINES) || \
                                                    ((__MODE__) == SPI_DIRECTION_1LINE))

/** @brief  Checks if SPI Data Size parameter is in allowed range.
  * @param  __DATASIZE__ specifies the SPI Data Size.
  *         This parameter can be a value of @ref SPI_Data_Size
  * @retval None
  */
#define IS_SPI_DATASIZE(__DATASIZE__) (((__DATASIZE__) == SPI_DATASIZE_16BIT) || \
                                       ((__DATASIZE__) == SPI_DATASIZE_8BIT))

/** @brief  Checks if SPI Serial clock steady state parameter is in allowed range.
  * @param  __CPOL__ specifies t
/* ... truncated ... */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/src/stm32f1xx_hal_spi.c`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_spi.c
  * @author  MCD Application Team
  * @brief   SPI HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Serial Peripheral Interface (SPI) peripheral:
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *           + Peripheral Control functions
  *           + Peripheral State functions
  *
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
    [..]
      The SPI HAL driver can be used as follows:

      (#) Declare a SPI_HandleTypeDef handle structure, for example:
          SPI_HandleTypeDef  hspi;

      (#)Initialize the SPI low level resources by implementing the HAL_SPI_MspInit() API:
          (##) Enable the SPIx interface clock
          (##) SPI pins configuration
              (+++) Enable the clock for the SPI GPIOs
              (+++) Configure these SPI pins as alternate function push-pull
          (##) NVIC configuration if you need to use interrupt process
              (+++) Configure the SPIx interrupt priority
              (+++) Enable the NVIC SPI IRQ handle
          (##) DMA Configuration if you need to use DMA process
              (+++) Declare a DMA_HandleTypeDef handle structure for the transmit or receive Stream/Channel
              (+++) Enable the DMAx clock
              (+++) Configure the DMA handle parameters
              (+++) Configure the DMA Tx or Rx Stream/Channel
              (+++) Associate the initialized hdma_tx(or _rx)  handle to the hspi DMA Tx or Rx handle
              (+++) Configure the priority and enable the NVIC for the transfer complete interrupt on the DMA Tx or Rx Stream/Channel

      (#) Program the Mode, BidirectionalMode , Data size, Baudrate Prescaler, NSS
          management, Clock polarity and phase, FirstBit and CRC configuration in the hspi Init structure.

      (#) Initialize the SPI registers by calling the HAL_SPI_Init() API:
          (++) This API configures also the low level Hardware GPIO, CLOCK, CORTEX...etc)
              by calling the customized HAL_SPI_MspInit() API.
     [..]
       Circular mode restriction:
      (#) The DMA circular mode cannot be used when the SPI is configured in these modes:
          (##) Master 2Lines RxOnly
          (##) Master 1Line Rx
      (#) The CRC feature is not managed when the DMA circular mode is enabled
      (#) When the SPI DMA Pause/Stop features are used, we must use the following APIs
          the HAL_SPI_DMAPause()/ HAL_SPI_DMAStop() only under the SPI callbacks
     [..]
       Master Receive mode restriction:
      (#) In Master unidirectional receive-only mode (MSTR =1, BIDIMODE=0, RXONLY=1) or
          bidirectional receive mode (MSTR=1, BIDIMODE=1, BIDIOE=0), to ensure that the SPI
          does not initiate a new transfer the following procedure has to be respected:
          (##) HAL_SPI_DeInit()
          (##) HAL_SPI_Init()
     [..]
       Callback registration:

      (#) The compilation flag USE_HAL_SPI_REGISTER_CALLBACKS when set to 1U
          allows the user to configure dynamically the driver callbacks.
          Use Functions HAL_SPI_RegisterCallback() to register an interrupt callback.

          Function HAL_SPI_RegisterCallback() allows to register following callbacks:
            (++) TxCpltCallback        : SPI Tx Completed callback
            (++) RxCpltCallback        : SPI Rx Completed callback
            (++) TxRxCpltCallback      : SPI TxRx Completed callback
            (++) TxHalfCpltCallback    : SPI Tx Half Completed callback
            (++) RxHalfCpltCallback    : SPI Rx Half Completed callback
            (++) TxRxHalfCpltCallback  : SPI TxRx Half Completed callback
            (++) ErrorCallback         : SPI Error callback
            (++) AbortCpltCallback     : SPI Abort callback
            (++) MspInitCallback       : SPI Msp Init callback
            (++) MspDeInitCallback     : SPI Msp DeInit callback
          This function takes as parameters the HAL peripheral handle, the Callback ID
          and a pointer to the user callback function.


      (#) Use function HAL_SPI_UnRegisterCallback to reset a callback to the default
          weak function.
          HAL_SPI_UnRegisterCallback takes as parameters the HAL peripheral handle,
          and the Callback ID.
          This function allows to reset following callbacks:
            (++) TxCpltCallback        : SPI Tx Completed callback
            (++) RxCpltCallback        : SPI Rx Completed callback
            (++) TxRxCpltCallback      : SPI TxRx Completed callback
            (++) TxHalfCpltCallback    : SPI Tx Half Completed callback
            (++) RxHalfCpltCallback    : SPI Rx Half Completed callback
            (++) TxRxHalfCpltCallback  : SPI TxRx Half Completed callback
            (++) ErrorCallback         : SPI Error callback
            (++) AbortCpltCallback     : SPI Abort callback
            (++) MspInitCallback       : SPI Msp Init callback
            (++) MspDeInitCallback     : SPI Msp DeInit callback

       [..]
       By default, after the HAL_SPI_Init() and when the state is HAL_SPI_STATE_RESET
       all callbacks are set to the corresponding weak functions:
       examples HAL_SPI_MasterTxCpltCallback(), HAL_SPI_MasterRxCpltCallback().
       Exception done for MspInit and MspDeInit functions that are
       reset to the legacy weak functions in the HAL_SPI_Init()/ HAL_SPI_DeInit() only when
       these callbacks are null (not registered beforehand).
       If MspInit or MspDeInit are not null, the HAL_SPI_Init()/ HAL_SPI_DeInit()
       keep and use the user MspInit/MspDeInit callbacks (registered beforehand) whatever the state.

       [..]
       Callbacks can be registered/unregistered in HAL_SPI_STATE_READY state only.
       Exception done MspInit/MspDeInit functions that can be registered/unregistered
       in HAL_SPI_STATE_READY or HAL_SPI_STATE_RESET state,
       thus registered (user) MspInit/DeInit callbacks can be used during the Init/DeInit.
       Then, the user first registers the MspInit/MspDeInit user callbacks
       using HAL_SPI_RegisterCallback() before calling HAL_SPI_DeInit()
       or HAL_SPI_Init() function.

       [..]
       When the compilation define USE_HAL_PPP_REGISTER_CALLBACKS is set to 0 or
       not defined, the callback registering feature is not available
       and weak (surcharged) callbacks are used.

     [..]
       Using the HAL it is not possible to reach all supported SPI frequency with the different SPI Modes,
       the following table resume the max SPI frequency reached with data size 8bits/16bits,
         according to frequency of the APBx Peripheral Clock (fPCLK) used by the SPI instance.

  @endverbatim

  Additional table :

       DataSize = SPI_DATASIZE_8BIT:
       +----------------------------------------------------------------------------------------------+
       |         |                | 2Lines Fullduplex   |     2Lines RxOnly    |         1Line        |
       | Process | Tranfert mode  |---------------------|----------------------|----------------------|
       |         |                |  Master  |  Slave   |  Master   |  Slave   |  Master   |  Slave   |
       |==============================================================================================|
       |    T    |     Polling    | Fpclk/2  | Fpclk/2  |    NA     |    NA    |    NA     |   NA     |
       |    X    |----------------|----------|----------|-----------|----------|-----------|----------|
       |    /    |     Interrupt  | Fpclk/4  | Fpclk/8  |    NA     |    NA    |    NA     |   NA     |
       |    R    |----------------|----------|----------|-----------|----------|-----------|----------|
       |    X    |       DMA      | Fpclk/2  | Fpclk/2  |    NA     |    NA    |    NA     |   NA     |
       |=========|================|==========|==========|===========|==========|===========|==========|
       |         |     Polling    | Fpclk/2  | Fpclk/2  | Fpclk/64  | Fpclk/2  | Fpclk/64  | Fpclk/2  |
       |         |----------------|----------|----------|-----------|----------|-----------|----------|
       |    R    |     Interrupt  | Fpclk/8  | Fpclk/8  | Fpclk/64  | Fpclk/2  | Fpclk/64  | Fpclk/2  |
       |    X    |----------------|----------|----------|-----------|----------|-----------|----------|
       |         |       DMA      | Fpclk/2  | Fpclk/2  | Fpclk/64  | Fpclk/2  | Fpclk/128 | Fpclk/2  |
       |=========|================|==========|==========|===========|==========|===========|==========|
       |         |     Polling    | Fpclk/2  | Fpclk/4  |     NA    |    NA    | Fpclk/2   | Fpclk/64 |
       |         |----------------|----------|----------|-----------|----------|-----------|----------|
       |    T    |     Interrupt  | Fpclk/2  | Fpclk/4  |     NA    |    NA    | Fpclk/2   | Fpclk/64 |
       |    X    |----------------|----------|----------|-----------|----------|-----------|----------|
       |         |       DMA      | Fpclk/2  | Fpclk/2  |     NA    |    NA    | Fpclk/2   | Fpclk/128|
       +----------------------------------------------------------------------------------------------+

       DataSize = SPI_DATASIZE_16BIT:
       +----------------------------------------------------------------------------------------------+
       |         |                | 2Lines Fullduplex   |     2Lines RxOnly    |         1Line        |
       | Process | Tranfert mode  |---------------------|----------------------|----------------------|
       |         |                |  Master  |  Slave   |  Master   |  Slave   |  Master   |  Slave   |
       |==============================================================================================|
       |    T    |     Polling    | Fpclk/2  | Fpclk/2  |    NA     |    NA    |    NA     |   NA     |
       |    X    |----------------|----------|----------|-----------|----------|-----------|----------|
       |    /    |     Interrupt  | Fpclk/4  | Fpclk/4  |    NA     |    NA    |    NA     |   NA     |
       |    R    |----------------|----------|----------|-----------|----------|-----------|----------|
       |    X    |       DMA      | Fpclk/2  | Fpclk/2  |    NA     |    NA    |    NA     |   NA     |
       |=========|================|==========|==========|===========|==========|===========|==========|
       |         |     Polling    | Fpclk/2  | Fpclk/2  | Fpclk/64  | Fpclk/2  | Fpclk/32  | Fpclk/2  |
       |         |----------------|----------|----------|-----------|----------|-----------|----------|
       |    R    |     Interrupt  | Fpclk/4  | Fpclk/4  | Fpclk/64  | Fpclk/2  | Fpclk/64  | Fpclk/2  |
       |    X    |----------------|----------|----------|-----------|----------|-----------|----------|
       |         |       DMA      | Fpclk/2  | Fpclk/2  | Fpclk/64  | Fpclk/2  | Fpclk/128 | Fpclk/2  |
       |=========|================|==========|==========|===========|==========|===========|==========|
       |         |     Polling    | Fpclk/2  | Fpclk/2  |     NA    |    NA    | Fpclk/2   | Fpclk/32 |
       |         |----------------|----------|----------|-----------|----------|-----------|----------|
       |    T    |     Interrupt  | Fpclk/2  | Fpclk/2  |     NA    |    NA    | Fpclk/2   | Fpclk/64 |
       |    X    |----------------|----------|----------|-----------|----------|-----------|----------|
       |         |       DMA      | Fpclk/2  | Fpclk/2  |     NA    |    NA    | Fpclk/2   | Fpclk/128|
       +----------------------------------------------------------------------------------------------+
       @note The max SPI frequency depend on SPI data size (8bits, 16bits),
             SPI mode(2 Lines fullduplex, 2 lines RxOnly, 1 line TX/RX) and Process mode (Polling, IT, DMA).
       @note
            (#) TX/RX processes are HAL_SPI_TransmitReceive(), HAL_SPI_TransmitReceive_IT() and HAL_SPI_TransmitReceive_DMA()
            (#) RX processes are HAL_SPI_Receive(), HAL_SPI_Receive_IT() and HAL_SPI_Receive_DMA()
            (#) TX processes are HAL_SPI_Transmit(), HAL_SPI_Transmit_IT() and HAL_SPI_Transmit_DMA()

  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @defgroup SPI SPI
  * @brief SPI HAL module driver
  * @{
  */
#ifdef HAL_SPI_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#if (USE_SPI_CRC != 0U) && defined(SPI_CRC_ERROR_WORKAROUND_FEATURE)
/* CRC WORKAOUND FEATURE: Variable used to determine if device is impacted by implementation
 * of workaround related to wrong CRC errors detection on SPI2. Conditions in which this workaround
 * has to be applied, are:
 *  - STM32F101CDE/STM32F103CDE
 *  - Revision ID : Z
 *  - SPI2
 *  - In receive only mode, with CRC calculation enabled, at the end of the CRC reception,
 *     the software needs to check the CRCERR flag. If it is found set, read back the SPI_RXCRC:
 *       + If the value is 0, the complete data transfer is successful.
 *       + Otherwise, one or more errors have been detected during the data transfer by CPU or DMA.
 *    If CRCERR is found reset, the complete data transfer is considered successful.
 *
 * Check RevisionID value for identifying if Device is Rev Z (0x0001) in order to enable workaround for
 * CRC errors wrongly detected
 */
/* Pb is that ES_STM32F10xxCDE also identify an issue in Debug registers access while not in Debug mode
 * Revision ID information is only available in Debug mode, so Workaround could not be implemented
 * to distinguish Rev Z devices (issue present) from more recent version (issue fixed).
 * So, in case of Revison Z F101 or F103 devices, below define should be assigned to 1.
 */
#define  USE_SPI_CRC_ERROR_WORKAROUND   0U
#endif
/** @defgroup SPI_Private_Constants SPI Private Constants
  * @{
  */
#define SPI_DEFAULT_TIMEOUT 100U
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup SPI_Private_Functions SPI Private Functions
  * @{
  */
static void SPI_DMATransmitCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMAReceiveCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMATransmitReceiveCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMAHalfTransmitCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMAHalfReceiveCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMAHalfTransmitReceiveCplt(DMA_HandleTypeDef *hdma);
static void SPI_DMAError(DMA_HandleTypeDef *hdma);
static void SPI_DMAAbortOnError(DMA_HandleTypeDef *hdma);
static void SPI_DMATxAbortCallback(DMA_HandleTypeDef *hdma);
static void SPI_DMARxAbortCallback(DMA_HandleTypeDef *hdma);
static HAL_StatusTypeDef SPI_WaitFlagStateUntilTimeout(SPI_HandleTypeDef *hspi, uint32_t Flag, FlagStatus State,
                                                       uint32_t Timeout, uint32_t Tickstart);
static void SPI_TxISR_8BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_TxISR_16BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_RxISR_8BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_RxISR_16BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_2linesRxISR_8BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_2linesTxISR_8BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_2linesTxISR_16BIT(struct __SPI_HandleTypeDef *hspi);
static void SPI_2linesRxISR_16BIT(struct __SPI_HandleTypeDef *hspi);
#if (USE_SPI_CRC != 0U)
static void SPI_RxISR_8BITCRC(struct __SPI_HandleTypeDef *hspi);
static void SPI_RxISR_16BITCRC(struct __SPI_HandleTypeDef *hspi);
static void SPI_2linesRxISR_8BITCRC(struct __SPI_HandleTypeDef *hspi);
static void SPI_2linesRxISR_16BITCRC(struct __SPI_HandleTypeDef *hspi);
#endif /* USE_SPI_CRC */
static void SPI_AbortRx_ISR(SPI_HandleTypeDef *hspi);
static void SPI_AbortTx_ISR(SPI_HandleTypeDef *hspi);
static void SPI_CloseRxTx_ISR(SPI_HandleTypeDef *hspi);
static void SPI_CloseRx_ISR(SPI_HandleTypeDef *hspi);
static void SPI_CloseTx_ISR(SPI_HandleTypeDef *hspi);
static HAL_StatusTypeDef SPI_EndRxTransaction(SPI_HandleTypeDef *hspi, uint32_t Timeout, uint32_t Tickstart);
static HAL_StatusTypeDef SPI_EndRxTxTransaction(SPI_HandleTypeDef *hspi, uint32_t Timeout, uint32_t Tickstart);
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup SPI_Exported_Functions SPI Exported Functions
  * @{
  */

/** @defgroup SPI_Exported_Functions_Group1 Initialization and de-initialization functions
  *  @brief    Initialization and Configuration functions
  *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]  This subsection provides a set of functions allowing to initialize and
          de-initialize the SPIx peripheral:

      (+) User must implement HAL_SPI_MspInit() function in which he configures
          all related peripherals resources (CLOCK, GPIO, DMA, IT and NVIC ).

      (+) Call the function HAL_SPI_Init() to configure the selected device with
          the selected configuration:
        (++) Mode
        (++) Direction
        (++) Data Size
        (++) Clock Polarity and Phase
        (++) NSS Management
        (++) BaudRate Prescaler
        (++) FirstBit
        (++) TIMode
        (++) CRC Calculation
        (++) CRC Polynomial if CRC enabled

      (+) Call the function HAL_SPI_DeInit() to restore the default configuration
          of the selected SPIx peripheral.

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the SPI according to the specified parameters
  *         in the SPI_InitTypeDef and initialize the associated handle.
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi)
{
  /* Check the SPI handle allocation */
  if (hspi == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_SPI_ALL_INSTANCE(hspi->Instance));
  assert_param(IS_SPI_MODE(hspi->Init.Mode));
  assert_param(IS_SPI_DIRECTION(hspi->Init.Direction));
  assert_param(IS_SPI_DATASIZE(hspi->Init.DataSize));
  assert_param(IS_SPI_NSS(hspi->Init.NSS));
  assert_param(IS_SPI_BAUDRATE_PRESCALER(hspi->Init.BaudRatePrescaler));
  assert_param(IS_SPI_FIRST_BIT(hspi->Init.FirstBit));
  /* TI mode is not supported on this device.
     TIMode parameter is mandatory equal to SPI_TIMODE_DISABLE */
  assert_param(IS_SPI_TIMODE(hspi->Init.TIMode));
  if (hspi->Init.TIMode == SPI_TIMODE_DISABLE)
  {
    assert_param(IS_SPI_CPOL(hspi->Init.CLKPolarity));
    assert_param(IS_SPI_CPHA(hspi->Init.CLKPhase));
  }
#if (USE_SPI_CRC != 0U)
  assert_param(IS_SPI_CRC_CALCULATION(hspi->Init.CRCCalculation));
  if (hspi->Init.CRCCalculation == SPI_CRCCALCULATION_ENABLE)
  {
    assert_param(IS_SPI_CRC_POLYNOMIAL(hspi->Init.CRCPolynomial));
  }
#else
  hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
#endif /* USE_SPI_CRC */

  if (hspi->State == HAL_SPI_STATE_RESET)
  {
    /* Allocate lock resource and initialize it */
    hspi->Lock = HAL_UNLOCKED;

#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
    /* Init the SPI Callback settings */
    hspi->TxCpltCallback       = HAL_SPI_TxCpltCallback;       /* Legacy weak TxCpltCallback       */
    hspi->RxCpltCallback       = HAL_SPI_RxCpltCallback;       /* Legacy weak RxCpltCallback       */
    hspi->TxRxCpltCallback     = HAL_SPI_TxRxCpltCallback;     /* Legacy weak TxRxCpltCallback     */
    hspi->TxHalfCpltCallback   = HAL_SPI_TxHalfCpltCallback;   /* Legacy weak TxHalfCpltCallback   */
    hspi->RxHalfCpltCallback   = HAL_SPI_RxHalfCpltCallback;   /* Legacy weak RxHalfCpltCallback   */
    hspi->TxRxHalfCpltCallback = HAL_SPI_TxRxHalfCpltCallback; /* Legacy weak TxRxHalfCpltCallback */
    hspi->ErrorCallback        = HAL_SPI_ErrorCallback;        /* Legacy weak ErrorCallback        */
    hspi->AbortCpltCallback    = HAL_SPI_AbortCpltCallback;    /* Legacy weak AbortCpltCallback    */

    if (hspi->MspInitCallback == NULL)
    {
      hspi->MspInitCallback = HAL_SPI_MspInit; /* Legacy weak MspInit  */
    }

    /* Init the low level hardware : GPIO, CLOCK, NVIC... */
    hspi->MspInitCallback(hspi);
#else
    /* Init the low level hardware : GPIO, CLOCK, NVIC... */
    HAL_SPI_MspInit(hspi);
#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
  }

  hspi->State = HAL_SPI_STATE_BUSY;

  /* Disable the selected SPI peripheral */
  __HAL_SPI_DISABLE(hspi);

  /*----------------------- SPIx CR1 & CR2 Configuration ---------------------*/
  /* Configure : SPI Mode, Communication Mode, Data size, Clock polarity and phase, NSS management,
  Communication speed, First bit and CRC calculation state */
  WRITE_REG(hspi->Instance->CR1, (hspi->Init.Mode | hspi->Init.Direction | hspi->Init.DataSize |
                                  hspi->Init.CLKPolarity | hspi->Init.CLKPhase | (hspi->Init.NSS & SPI_CR1_SSM) |
                                  hspi->Init.BaudRatePrescaler | hspi->Init.FirstBit  | hspi->Init.CRCCalculation));

  /* Configure : NSS management */
  WRITE_REG(hspi->Instance->CR2, ((hspi->Init.NSS >> 16U) & SPI_CR2_SSOE));

#if (USE_SPI_CRC != 0U)
  /*---------------------------- SPIx CRCPOLY Configuration ------------------*/
  /* Configure : CRC Polynomial */
  if (hspi->Init.CRCCalculation == SPI_CRCCALCULATION_ENABLE)
  {
    WRITE_REG(hspi->Instance->CRCPR, hspi->Init.CRCPolynomial);
  }
#endif /* USE_SPI_CRC */

#if defined(SPI_I2SCFGR_I2SMOD)
  /* Activate the SPI mode (Make sure that I2SMOD bit in I2SCFGR register is reset) */
  CLEAR_BIT(hspi->Instance->I2SCFGR, SPI_I2SCFGR_I2SMOD);
#endif /* SPI_I2SCFGR_I2SMOD */

  hspi->ErrorCode = HAL_SPI_ERROR_NONE;
  hspi->State     = HAL_SPI_STATE_READY;

  return HAL_OK;
}

/**
  * @brief  De-Initialize the SPI peripheral.
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef *hspi)
{
  /* Check the SPI handle allocation */
  if (hspi == NULL)
  {
    return HAL_ERROR;
  }

  /* Check SPI Instance parameter */
  assert_param(IS_SPI_ALL_INSTANCE(hspi->Instance));

  hspi->State = HAL_SPI_STATE_BUSY;

  /* Disable the SPI Peripheral Clock */
  __HAL_SPI_DISABLE(hspi);

#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
  if (hspi->MspDeInitCallback == NULL)
  {
    hspi->MspDeInitCallback = HAL_SPI_MspDeInit; /* Legacy weak MspDeInit  */
  }

  /* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
  hspi->MspDeInitCallback(hspi);
#else
  /* DeInit the low level hardware: GPI
/* ... truncated ... */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/stm32f1xx_hal_gpio.h`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_gpio.h
  * @author  MCD Application Team
  * @brief   Header file of GPIO HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32F1xx_HAL_GPIO_H
#define STM32F1xx_HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal_def.h"

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @addtogroup GPIO
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup GPIO_Exported_Types GPIO Exported Types
  * @{
  */

/**
  * @brief GPIO Init structure definition
  */
typedef struct
{
  uint32_t Pin;       /*!< Specifies the GPIO pins to be configured.
                           This parameter can be any value of @ref GPIO_pins_define */

  uint32_t Mode;      /*!< Specifies the operating mode for the selected pins.
                           This parameter can be a value of @ref GPIO_mode_define */

  uint32_t Pull;      /*!< Specifies the Pull-up or Pull-Down activation for the selected pins.
                           This parameter can be a value of @ref GPIO_pull_define */

  uint32_t Speed;     /*!< Specifies the speed for the selected pins.
                           This parameter can be a value of @ref GPIO_speed_define */
} GPIO_InitTypeDef;

/**
  * @brief  GPIO Bit SET and Bit RESET enumeration
  */
typedef enum
{
  GPIO_PIN_RESET = 0u,
  GPIO_PIN_SET
} GPIO_PinState;
/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup GPIO_Exported_Constants GPIO Exported Constants
  * @{
  */

/** @defgroup GPIO_pins_define GPIO pins define
  * @{
  */
#define GPIO_PIN_0                 ((uint16_t)0x0001)  /* Pin 0 selected    */
#define GPIO_PIN_1                 ((uint16_t)0x0002)  /* Pin 1 selected    */
#define GPIO_PIN_2                 ((uint16_t)0x0004)  /* Pin 2 selected    */
#define GPIO_PIN_3                 ((uint16_t)0x0008)  /* Pin 3 selected    */
#define GPIO_PIN_4                 ((uint16_t)0x0010)  /* Pin 4 selected    */
#define GPIO_PIN_5                 ((uint16_t)0x0020)  /* Pin 5 selected    */
#define GPIO_PIN_6                 ((uint16_t)0x0040)  /* Pin 6 selected    */
#define GPIO_PIN_7                 ((uint16_t)0x0080)  /* Pin 7 selected    */
#define GPIO_PIN_8                 ((uint16_t)0x0100)  /* Pin 8 selected    */
#define GPIO_PIN_9                 ((uint16_t)0x0200)  /* Pin 9 selected    */
#define GPIO_PIN_10                ((uint16_t)0x0400)  /* Pin 10 selected   */
#define GPIO_PIN_11                ((uint16_t)0x0800)  /* Pin 11 selected   */
#define GPIO_PIN_12                ((uint16_t)0x1000)  /* Pin 12 selected   */
#define GPIO_PIN_13                ((uint16_t)0x2000)  /* Pin 13 selected   */
#define GPIO_PIN_14                ((uint16_t)0x4000)  /* Pin 14 selected   */
#define GPIO_PIN_15                ((uint16_t)0x8000)  /* Pin 15 selected   */
#define GPIO_PIN_All               ((uint16_t)0xFFFF)  /* All pins selected */

#define GPIO_PIN_MASK              0x0000FFFFu /* PIN mask for assert test */
/**
  * @}
  */

/** @defgroup GPIO_mode_define GPIO mode define
  * @brief GPIO Configuration Mode
  *        Elements values convention: 0xX0yz00YZ
  *           - X  : GPIO mode or EXTI Mode
  *           - y  : External IT or Event trigger detection
  *           - z  : IO configuration on External IT or Event
  *           - Y  : Output type (Push Pull or Open Drain)
  *           - Z  : IO Direction mode (Input, Output, Alternate or Analog)
  * @{
  */
#define  GPIO_MODE_INPUT                        0x00000000u   /*!< Input Floating Mode                   */
#define  GPIO_MODE_OUTPUT_PP                    0x00000001u   /*!< Output Push Pull Mode                 */
#define  GPIO_MODE_OUTPUT_OD                    0x00000011u   /*!< Output Open Drain Mode                */
#define  GPIO_MODE_AF_PP                        0x00000002u   /*!< Alternate Function Push Pull Mode     */
#define  GPIO_MODE_AF_OD                        0x00000012u   /*!< Alternate Function Open Drain Mode    */
#define  GPIO_MODE_AF_INPUT                     GPIO_MODE_INPUT          /*!< Alternate Function Input Mode         */

#define  GPIO_MODE_ANALOG                       0x00000003u   /*!< Analog Mode  */

#define  GPIO_MODE_IT_RISING                    0x10110000u   /*!< External Interrupt Mode with Rising edge trigger detection          */
#define  GPIO_MODE_IT_FALLING                   0x10210000u   /*!< External Interrupt Mode with Falling edge trigger detection         */
#define  GPIO_MODE_IT_RISING_FALLING            0x10310000u   /*!< External Interrupt Mode with Rising/Falling edge trigger detection  */

#define  GPIO_MODE_EVT_RISING                   0x10120000u   /*!< External Event Mode with Rising edge trigger detection               */
#define  GPIO_MODE_EVT_FALLING                  0x10220000u   /*!< External Event Mode with Falling edge trigger detection              */
#define  GPIO_MODE_EVT_RISING_FALLING           0x10320000u   /*!< External Event Mode with Rising/Falling edge trigger detection       */

/**
  * @}
  */

/** @defgroup GPIO_speed_define  GPIO speed define
  * @brief GPIO Output Maximum frequency
  * @{
  */
#define  GPIO_SPEED_FREQ_LOW              (GPIO_CRL_MODE0_1) /*!< Low speed */
#define  GPIO_SPEED_FREQ_MEDIUM           (GPIO_CRL_MODE0_0) /*!< Medium speed */
#define  GPIO_SPEED_FREQ_HIGH             (GPIO_CRL_MODE0)   /*!< High speed */

/**
  * @}
  */

/** @defgroup GPIO_pull_define GPIO pull define
  * @brief GPIO Pull-Up or Pull-Down Activation
  * @{
  */
#define  GPIO_NOPULL        0x00000000u   /*!< No Pull-up or Pull-down activation  */
#define  GPIO_PULLUP        0x00000001u   /*!< Pull-up activation                  */
#define  GPIO_PULLDOWN      0x00000002u   /*!< Pull-down activation                */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup GPIO_Exported_Macros GPIO Exported Macros
  * @{
  */

/**
  * @brief  Checks whether the specified EXTI line flag is set or not.
  * @param  __EXTI_LINE__: specifies the EXTI line flag to check.
  *         This parameter can be GPIO_PIN_x where x can be(0..15)
  * @retval The new state of __EXTI_LINE__ (SET or RESET).
  */
#define __HAL_GPIO_EXTI_GET_FLAG(__EXTI_LINE__) (EXTI->PR & (__EXTI_LINE__))

/**
  * @brief  Clears the EXTI's line pending flags.
  * @param  __EXTI_LINE__: specifies the EXTI lines flags to clear.
  *         This parameter can be any combination of GPIO_PIN_x where x can be (0..15)
  * @retval None
  */
#define __HAL_GPIO_EXTI_CLEAR_FLAG(__EXTI_LINE__) (EXTI->PR = (__EXTI_LINE__))

/**
  * @brief  Checks whether the specified EXTI line is asserted or not.
  * @param  __EXTI_LINE__: specifies the EXTI line to check.
  *          This parameter can be GPIO_PIN_x where x can be(0..15)
  * @retval The new state of __EXTI_LINE__ (SET or RESET).
  */
#define __HAL_GPIO_EXTI_GET_IT(__EXTI_LINE__) (EXTI->PR & (__EXTI_LINE__))

/**
  * @brief  Clears the EXTI's line pending bits.
  * @param  __EXTI_LINE__: specifies the EXTI lines to clear.
  *          This parameter can be any combination of GPIO_PIN_x where x can be (0..15)
  * @retval None
  */
#define __HAL_GPIO_EXTI_CLEAR_IT(__EXTI_LINE__) (EXTI->PR = (__EXTI_LINE__))

/**
  * @brief  Generates a Software interrupt on selected EXTI line.
  * @param  __EXTI_LINE__: specifies the EXTI line to check.
  *          This parameter can be GPIO_PIN_x where x can be(0..15)
  * @retval None
  */
#define __HAL_GPIO_EXTI_GENERATE_SWIT(__EXTI_LINE__) (EXTI->SWIER |= (__EXTI_LINE__))
/**
  * @}
  */

/* Include GPIO HAL Extension module */
#include "stm32f1xx_hal_gpio_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup GPIO_Exported_Functions
  * @{
  */

/** @addtogroup GPIO_Exported_Functions_Group1
  * @{
  */
/* Initialization and de-initialization functions *****************************/
void  HAL_GPIO_Init(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void  HAL_GPIO_DeInit(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin);
/**
  * @}
  */

/** @addtogroup GPIO_Exported_Functions_Group2
  * @{
  */
/* IO operation functions *****************************************************/
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
HAL_StatusTypeDef HAL_GPIO_LockPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/**
  * @}
  */

/**
  * @}
  */
/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup GPIO_Private_Constants GPIO Private Constants
  * @{
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup GPIO_Private_Macros GPIO Private Macros
  * @{
  */
#define IS_GPIO_PIN_ACTION(ACTION) (((ACTION) == GPIO_PIN_RESET) || ((ACTION) == GPIO_PIN_SET))
#define IS_GPIO_PIN(PIN)           (((((uint32_t)PIN) & GPIO_PIN_MASK ) != 0x00u) && ((((uint32_t)PIN) & ~GPIO_PIN_MASK) == 0x00u))
#define IS_GPIO_MODE(MODE) (((MODE) == GPIO_MODE_INPUT)              ||\
                            ((MODE) == GPIO_MODE_OUTPUT_PP)          ||\
                            ((MODE) == GPIO_MODE_OUTPUT_OD)          ||\
                            ((MODE) == GPIO_MODE_AF_PP)              ||\
                            ((MODE) == GPIO_MODE_AF_OD)              ||\
                            ((MODE) == GPIO_MODE_IT_RISING)          ||\
                            ((MODE) == GPIO_MODE_IT_FALLING)         ||\
                            ((MODE) == GPIO_MODE_IT_RISING_FALLING)  ||\
                            ((MODE) == GPIO_MODE_EVT_RISING)         ||\
                            ((MODE) == GPIO_MODE_EVT_FALLING)        ||\
                            ((MODE) == GPIO_MODE_EVT_RISING_FALLING) ||\
                            ((MODE) == GPIO_MODE_ANALOG))
#define IS_GPIO_SPEED(SPEED) (((SPEED) == GPIO_SPEED_FREQ_LOW) || \
                              ((SPEED) == GPIO_SPEED_FREQ_MEDIUM) || ((SPEED) == GPIO_SPEED_FREQ_HIGH))
#define IS_GPIO_PULL(PULL) (((PULL) == GPIO_NOPULL) || ((PULL) == GPIO_PULLUP) || \
                            ((PULL) == GPIO_PULLDOWN))
/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/
/** @defgroup GPIO_Private_Functions GPIO Private Functions
  * @{
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32F1xx_HAL_GPIO_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/src/stm32f1xx_hal_gpio.c`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_gpio.c
  * @author  MCD Application Team
  * @brief   GPIO HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the General Purpose Input/Output (GPIO) peripheral:
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *
  @verbatim
  ==============================================================================
                    ##### GPIO Peripheral features #####
  ==============================================================================
  [..]
  Subject to the specific hardware characteristics of each I/O port listed in the datasheet, each
  port bit of the General Purpose IO (GPIO) Ports, can be individually configured by software
  in several modes:
  (+) Input mode
  (+) Analog mode
  (+) Output mode
  (+) Alternate function mode
  (+) External interrupt/event lines

  [..]
  During and just after reset, the alternate functions and external interrupt
  lines are not active and the I/O ports are configured in input floating mode.

  [..]
  All GPIO pins have weak internal pull-up and pull-down resistors, which can be
  activated or not.

  [..]
  In Output or Alternate mode, each IO can be configured on open-drain or push-pull
  type and the IO speed can be selected depending on the VDD value.

  [..]
  All ports have external interrupt/event capability. To use external interrupt
  lines, the port must be configured in input mode. All available GPIO pins are
  connected to the 16 external interrupt/event lines from EXTI0 to EXTI15.

  [..]
  The external interrupt/event controller consists of up to 20 edge detectors in connectivity
  line devices, or 19 edge detectors in other devices for generating event/interrupt requests.
  Each input line can be independently configured to select the type (event or interrupt) and
  the corresponding trigger event (rising or falling or both). Each line can also masked
  independently. A pending register maintains the status line of the interrupt requests

                     ##### How to use this driver #####
  ==============================================================================
 [..]
   (#) Enable the GPIO APB2 clock using the following function : __HAL_RCC_GPIOx_CLK_ENABLE().

   (#) Configure the GPIO pin(s) using HAL_GPIO_Init().
       (++) Configure the IO mode using "Mode" member from GPIO_InitTypeDef structure
       (++) Activate Pull-up, Pull-down resistor using "Pull" member from GPIO_InitTypeDef
            structure.
       (++) In case of Output or alternate function mode selection: the speed is
            configured through "Speed" member from GPIO_InitTypeDef structure
       (++) Analog mode is required when a pin is to be used as ADC channel
            or DAC output.
       (++) In case of external interrupt/event selection the "Mode" member from
            GPIO_InitTypeDef structure select the type (interrupt or event) and
            the corresponding trigger event (rising or falling or both).

   (#) In case of external interrupt/event mode selection, configure NVIC IRQ priority
       mapped to the EXTI line using HAL_NVIC_SetPriority() and enable it using
       HAL_NVIC_EnableIRQ().

   (#) To get the level of a pin configured in input mode use HAL_GPIO_ReadPin().

   (#) To set/reset the level of a pin configured in output mode use
       HAL_GPIO_WritePin()/HAL_GPIO_TogglePin().

   (#) To lock pin configuration until next reset use HAL_GPIO_LockPin().

   (#) During and just after reset, the alternate functions are not
       active and the GPIO pins are configured in input floating mode (except JTAG
       pins).

   (#) The LSE oscillator pins OSC32_IN and OSC32_OUT can be used as general purpose
       (PC14 and PC15, respectively) when the LSE oscillator is off. The LSE has
       priority over the GPIO function.

   (#) The HSE oscillator pins OSC_IN/OSC_OUT can be used as
       general purpose PD0 and PD1, respectively, when the HSE oscillator is off.
       The HSE has priority over the GPIO function.

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @defgroup GPIO GPIO
  * @brief GPIO HAL module driver
  * @{
  */

#ifdef HAL_GPIO_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @addtogroup GPIO_Private_Constants GPIO Private Constants
  * @{
  */
#define GPIO_MODE             0x00000003u
#define EXTI_MODE             0x10000000u
#define GPIO_MODE_IT          0x00010000u
#define GPIO_MODE_EVT         0x00020000u
#define RISING_EDGE           0x00100000u
#define FALLING_EDGE          0x00200000u
#define GPIO_OUTPUT_TYPE      0x00000010u

#define GPIO_NUMBER           16u

/* Definitions for bit manipulation of CRL and CRH register */
#define  GPIO_CR_MODE_INPUT         0x00000000u /*!< 00: Input mode (reset state)  */
#define  GPIO_CR_CNF_ANALOG         0x00000000u /*!< 00: Analog mode  */
#define  GPIO_CR_CNF_INPUT_FLOATING 0x00000004u /*!< 01: Floating input (reset state)  */
#define  GPIO_CR_CNF_INPUT_PU_PD    0x00000008u /*!< 10: Input with pull-up / pull-down  */
#define  GPIO_CR_CNF_GP_OUTPUT_PP   0x00000000u /*!< 00: General purpose output push-pull  */
#define  GPIO_CR_CNF_GP_OUTPUT_OD   0x00000004u /*!< 01: General purpose output Open-drain  */
#define  GPIO_CR_CNF_AF_OUTPUT_PP   0x00000008u /*!< 10: Alternate function output Push-pull  */
#define  GPIO_CR_CNF_AF_OUTPUT_OD   0x0000000Cu /*!< 11: Alternate function output Open-drain  */

/**
  * @}
  */
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @defgroup GPIO_Exported_Functions GPIO Exported Functions
  * @{
  */

/** @defgroup GPIO_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
  [..]
    This section provides functions allowing to initialize and de-initialize the GPIOs
    to be ready for use.

@endverbatim
  * @{
  */


/**
  * @brief  Initializes the GPIOx peripheral according to the specified parameters in the GPIO_Init.
  * @param  GPIOx: where x can be (A..G depending on device used) to select the GPIO peripheral
  * @param  GPIO_Init: pointer to a GPIO_InitTypeDef structure that contains
  *         the configuration information for the specified GPIO peripheral.
  * @retval None
  */
void HAL_GPIO_Init(GPIO_TypeDef  *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
  uint32_t position = 0x00u;
  uint32_t ioposition;
  uint32_t iocurrent;
  uint32_t temp;
  uint32_t config = 0x00u;
  __IO uint32_t *configregister; /* Store the address of CRL or CRH register based on pin number */
  uint32_t registeroffset;       /* offset used during computation of CNF and MODE bits placement inside CRL or CRH register */

  /* Check the parameters */
  assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));
  assert_param(IS_GPIO_PIN(GPIO_Init->Pin));
  assert_param(IS_GPIO_MODE(GPIO_Init->Mode));

  /* Configure the port pins */
  while (((GPIO_Init->Pin) >> position) != 0x00u)
  {
    /* Get the IO position */
    ioposition = (0x01uL << position);

    /* Get the current IO position */
    iocurrent = (uint32_t)(GPIO_Init->Pin) & ioposition;

    if (iocurrent == ioposition)
    {
      /* Check the Alternate function parameters */
      assert_param(IS_GPIO_AF_INSTANCE(GPIOx));

      /* Based on the required mode, filling config variable with MODEy[1:0] and CNFy[3:2] corresponding bits */
      switch (GPIO_Init->Mode)
      {
        /* If we are configuring the pin in OUTPUT push-pull mode */
        case GPIO_MODE_OUTPUT_PP:
          /* Check the GPIO speed parameter */
          assert_param(IS_GPIO_SPEED(GPIO_Init->Speed));
          config = GPIO_Init->Speed + GPIO_CR_CNF_GP_OUTPUT_PP;
          break;

        /* If we are configuring the pin in OUTPUT open-drain mode */
        case GPIO_MODE_OUTPUT_OD:
          /* Check the GPIO speed parameter */
          assert_param(IS_GPIO_SPEED(GPIO_Init->Speed));
          config = GPIO_Init->Speed + GPIO_CR_CNF_GP_OUTPUT_OD;
          break;

        /* If we are configuring the pin in ALTERNATE FUNCTION push-pull mode */
        case GPIO_MODE_AF_PP:
          /* Check the GPIO speed parameter */
          assert_param(IS_GPIO_SPEED(GPIO_Init->Speed));
          config = GPIO_Init->Speed + GPIO_CR_CNF_AF_OUTPUT_PP;
          break;

        /* If we are configuring the pin in ALTERNATE FUNCTION open-drain mode */
        case GPIO_MODE_AF_OD:
          /* Check the GPIO speed parameter */
          assert_param(IS_GPIO_SPEED(GPIO_Init->Speed));
          config = GPIO_Init->Speed + GPIO_CR_CNF_AF_OUTPUT_OD;
          break;

        /* If we are configuring the pin in INPUT (also applicable to EVENT and IT mode) */
        case GPIO_MODE_INPUT:
        case GPIO_MODE_IT_RISING:
        case GPIO_MODE_IT_FALLING:
        case GPIO_MODE_IT_RISING_FALLING:
        case GPIO_MODE_EVT_RISING:
        case GPIO_MODE_EVT_FALLING:
        case GPIO_MODE_EVT_RISING_FALLING:
          /* Check the GPIO pull parameter */
          assert_param(IS_GPIO_PULL(GPIO_Init->Pull));
          if (GPIO_Init->Pull == GPIO_NOPULL)
          {
            config = GPIO_CR_MODE_INPUT + GPIO_CR_CNF_INPUT_FLOATING;
          }
          else if (GPIO_Init->Pull == GPIO_PULLUP)
          {
            config = GPIO_CR_MODE_INPUT + GPIO_CR_CNF_INPUT_PU_PD;

            /* Set the corresponding ODR bit */
            GPIOx->BSRR = ioposition;
          }
          else /* GPIO_PULLDOWN */
          {
            config = GPIO_CR_MODE_INPUT + GPIO_CR_CNF_INPUT_PU_PD;

            /* Reset the corresponding ODR bit */
            GPIOx->BRR = ioposition;
          }
          break;

        /* If we are configuring the pin in INPUT analog mode */
        case GPIO_MODE_ANALOG:
          config = GPIO_CR_MODE_INPUT + GPIO_CR_CNF_ANALOG;
          break;

        /* Parameters are checked with assert_param */
        default:
          break;
      }

      /* Check if the current bit belongs to first half or last half of the pin count number
       in order to address CRH or CRL register*/
      configregister = (iocurrent < GPIO_PIN_8) ? &GPIOx->CRL     : &GPIOx->CRH;
      registeroffset = (iocurrent < GPIO_PIN_8) ? (position << 2u) : ((position - 8u) << 2u);

      /* Apply the new configuration of the pin to the register */
      MODIFY_REG((*configregister), ((GPIO_CRL_MODE0 | GPIO_CRL_CNF0) << registeroffset), (config << registeroffset));

      /*--------------------- EXTI Mode Configuration ------------------------*/
      /* Configure the External Interrupt or event for the current IO */
      if ((GPIO_Init->Mode & EXTI_MODE) == EXTI_MODE)
      {
        /* Enable AFIO Clock */
        __HAL_RCC_AFIO_CLK_ENABLE();
        temp = AFIO->EXTICR[position >> 2u];
        CLEAR_BIT(temp, (0x0Fu) << (4u * (position & 0x03u)));
        SET_BIT(temp, (GPIO_GET_INDEX(GPIOx)) << (4u * (position & 0x03u)));
        AFIO->EXTICR[position >> 2u] = temp;


        /* Configure the interrupt mask */
        if ((GPIO_Init->Mode & GPIO_MODE_IT) == GPIO_MODE_IT)
        {
          SET_BIT(EXTI->IMR, iocurrent);
        }
        else
        {
          CLEAR_BIT(EXTI->IMR, iocurrent);
        }

        /* Configure the event mask */
        if ((GPIO_Init->Mode & GPIO_MODE_EVT) == GPIO_MODE_EVT)
        {
          SET_BIT(EXTI->EMR, iocurrent);
        }
        else
        {
          CLEAR_BIT(EXTI->EMR, iocurrent);
        }

        /* Enable or disable the rising trigger */
        if ((GPIO_Init->Mode & RISING_EDGE) == RISING_EDGE)
        {
          SET_BIT(EXTI->RTSR, iocurrent);
        }
        else
        {
          CLEAR_BIT(EXTI->RTSR, iocurrent);
        }

        /* Enable or disable the falling trigger */
        if ((GPIO_Init->Mode & FALLING_EDGE) == FALLING_EDGE)
        {
          SET_BIT(EXTI->FTSR, iocurrent);
        }
        else
        {
          CLEAR_BIT(EXTI->FTSR, iocurrent);
        }
      }
    }

	position++;
  }
}

/**
  * @brief  De-initializes the GPIOx peripheral registers to their default reset values.
  * @param  GPIOx: where x can be (A..G depending on device used) to select the GPIO peripheral
  * @param  GPIO_Pin: specifies the port bit to be written.
  *         This parameter can be one of GPIO_PIN_x where x can be (0..15).
  * @retval None
  */
void HAL_GPIO_DeInit(GPIO_TypeDef  *GPIOx, uint32_t GPIO_Pin)
{
  uint32_t position = 0x00u;
  uint32_t iocurrent;
  uint32_t tmp;
  __IO uint32_t *configregister; /* Store the address of CRL or CRH register based on pin number */
  uint32_t registeroffset;

  /* Check the parameters */
  assert_param(IS_GPIO_ALL_INSTANCE(GPIOx));
  assert_param(IS_GPIO_PIN(GPIO_Pin));

  /* Configure the port pins */
  while ((GPIO_Pin >> position) != 0u)
  {
    /* Get current io position */
    iocurrent = (GPIO_Pin) & (1uL << position);

    if (iocurrent)
    {
      /*------------------------- EXTI Mode Configuration --------------------*/
      /* Clear the External Interrupt or Event for the current IO */

      tmp = AFIO->EXTICR[position >> 2u];
      tmp &= 0x0FuL << (4u * (position & 0x03u));
      if (tmp == (GPIO_GET_INDEX(GPIOx) << (4u * (position & 0x03u))))
      {
        tmp = 0x0FuL << (4u * (position & 0x03u));
        CLEAR_BIT(AFIO->EXTICR[position >> 2u], tmp);

        /* Clear EXTI line configuration */
        CLEAR_BIT(EXTI->IMR, (uint32_t)iocurrent);
        CLEAR_BIT(EXTI->EMR, (uint32_t)iocurrent);

        /* Clear Rising Falling edge configuration */
        CLEAR_BIT(EXTI->RTSR, (uint32_t)iocurrent);
        CLEAR_BIT(EXTI->FTSR, (uint32_t)iocurrent);
      }
      /*------------------------- GPIO Mode Configuration --------------------*/
      /* Check if the current bit belongs to first half or last half of the pin count number
       in order to address CRH or CRL register */
      configregister = (iocurrent < GPIO_PIN_8) ? &GPIOx->CRL     : &GPIOx->CRH;
      registeroffset = (iocurrent < GPIO_PIN_8) ? (position << 2u) : ((position - 8u) << 2u);

      /* CRL/CRH default value is floating input(0x04) shifted to correct position */
      MODIFY_REG(*configregister, ((GPIO_CRL_MODE0 | GPIO_CRL_CNF0) << registeroffset), GPIO_CRL_CNF0_0 << registeroffset);

      /* ODR default value is 0 */
      CLEAR_BIT(GPIOx->ODR, iocurrent);
    }

    position++;
  }
}

/**
  * @}
  */

/** @defgroup GPIO_Exported_Functions_Group2 IO operation functions
 *  @brief   GPIO Read and Write
 *
@verbatim
 ===============================================================================
                       ##### IO operation functions #####
 ===============================================================================
  [..]
    This subsection provides a set of functions allowing to manage the GPIOs.

@endverbatim
  * @{
  */

/**
  * @brief  Reads the specified input port pin.
  * @param  GPIOx: where x can be (A..G depending on device used) to select the GPIO peripheral
  * @param  GPIO_Pin: specifies the port bit to read.
  *         This parameter can be GPIO_PIN_x where x can be (0..15).
  * @retval The input port pin value.
  */
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  GPIO_PinState bitstatus;

  /* Check the parameters */
  assert_param(IS_GPIO_PIN(GPIO_Pin));

  if ((GPIOx->IDR & GPIO_Pin) != (uint32_t)GPIO_PIN_RESET)
  {
    bitstatus = GPIO_PIN_SET;
  }
  else
  {
    bitstatus = GPIO_PIN_RESET;
  }
  return bitstatus;
}

/**
  * @brief  Sets or clears the selected data port bit.
  *
  * @note   This function uses GPIOx_BSRR register to allow atomic read/modify
  *         accesses. In this way, there is no risk of an IRQ occurring between
  *         the read and the modify access.
  *
  * @param  GPIOx: where x can be (A..G depending on device used) to select the GPIO peripheral
  * @param  GPIO_Pin: specifies the port bit to be written.
  *          This parameter can be one of GPIO_PIN_x where x can be (0..15).
  * @param  PinState: specifies the value to be written to the selected bit.
  *          This parameter can be one of the GPIO_PinState enum values:
  *            @arg GPIO_PIN_RESET: to clear the port pin
  *            @arg GPIO_PIN_SET: to set the port pin
  * @retval None
  */
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
  /* Check the parameters */
  assert_param(IS_GPIO_PIN(GPIO_Pin));
  assert_param(IS_GPIO_PIN_ACTION(PinState));

  if (PinState != GPIO_PIN_RESET)
  {
    GPIOx->BSRR = GPIO_Pin;
  }
  else
  {
    GPIOx->BSRR = (uint32_t)GPIO_Pin << 16u;
  }
}

/**
  * @brief  Toggles the specified GPIO pin
  * @param  GPIOx: where x can be (A..G depending on device used) to select the GPIO peripheral
  * @param  GPIO_Pin: Specifies the pins to be toggled.
  * @retval None
  */
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  /* Check the parameters */
  assert_param(IS_GPIO_PIN(GPIO_Pin));

  if ((GPIOx->ODR & GPIO_Pin) != 0x00u)
  {
    GPIOx->BRR = (uint32_t)GPIO_Pin;
  }
  else
  {
    GPIOx->BSRR = (uint32_t)GPIO_Pin;
  }
}

/**
* @brief  Locks GPIO Pins configuration registers.
* @note   The locking mechanism allows the IO configuration to be frozen. When the LOCK sequence
*         has been applied on a port bit, it is no longer possible to modify the value of the port bit until
*         the next reset.
* @param  GPIOx: where x can be (A..G depending on device used) to select the GPIO peripheral
* @param  GPIO_Pin: specifies the port bit to be locked.
*         This parameter can be any combination of GPIO_Pin_x where x can be (0..15).
* @retval None
*/
HAL_StatusTypeDef HAL_GPIO_LockPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  __IO uint32_t tmp = GPIO_LCKR_LCKK;

  /* Check the parameters */
  assert_param(IS_GPIO_LOCK_INSTANCE(GPIOx));
  assert_param(IS_GPIO_PIN(GPIO_Pin));

  /* Apply lock key write sequence */
  SET_BIT(tmp, GPIO_Pin);
  /* Set LCKx bit(s): LCKK='1' + LCK[15-0] */
  GPIOx->LCKR = tmp;
  /* Reset LCKx bit(s): LCKK='0' + LCK[15-0] */
  GPIOx->LCKR = GPIO_Pin;
  /* Set LCKx bit(s): LCKK='1' + LCK[15-0] */
  GPIOx->LCKR = tmp;
  /* Read LCKK register. This read is mandatory to complete key lock sequence */
  tmp = GPIOx->LCKR;

  /* read again in order to confirm lock is active */
  if ((uint32_t)(GPIOx->LCKR & GPIO_LCKR_LCKK))
  {
    return HAL_OK;
  }
  else
  {
    return HAL_ERROR;
  }
}

/**
  * @brief  This function handles EXTI interrupt request.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)
{
  /* EXTI line interrupt detected */
  if (__HAL_GPIO_EXTI_GET_IT(GPIO_Pin) != 0x00u)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
    HAL_GPIO_EXTI_Callback(GPIO_Pin);
  }
}

/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
__weak void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(GPIO_Pin);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_GPIO_EXTI_Callback could be implemented in the user file
   */
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_GPIO_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/stm32f1xx_hal.h`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the HAL
  *          module driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1xx_HAL_H
#define __STM32F1xx_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal_conf.h"

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @addtogroup HAL
  * @{
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup HAL_Exported_Constants HAL Exported Constants
  * @{
  */

/** @defgroup HAL_TICK_FREQ Tick Frequency
  * @{
  */
typedef enum
{
  HAL_TICK_FREQ_10HZ         = 100U,
  HAL_TICK_FREQ_100HZ        = 10U,
  HAL_TICK_FREQ_1KHZ         = 1U,
  HAL_TICK_FREQ_DEFAULT      = HAL_TICK_FREQ_1KHZ
} HAL_TickFreqTypeDef;
/**
  * @}
  */
/* Exported types ------------------------------------------------------------*/
extern uint32_t uwTickPrio;
extern HAL_TickFreqTypeDef uwTickFreq;

/**
  * @}
  */
/* Exported macro ------------------------------------------------------------*/
/** @defgroup HAL_Exported_Macros HAL Exported Macros
  * @{
  */

/** @defgroup DBGMCU_Freeze_Unfreeze Freeze Unfreeze Peripherals in Debug mode
  * @brief   Freeze/Unfreeze Peripherals in Debug mode
  * Note: On devices STM32F10xx8 and STM32F10xxB,
  *                  STM32F101xC/D/E and STM32F103xC/D/E,
  *                  STM32F101xF/G and STM32F103xF/G
  *                  STM32F10xx4 and STM32F10xx6
  *       Debug registers DBGMCU_IDCODE and DBGMCU_CR are accessible only in
  *       debug mode (not accessible by the user software in normal mode).
  *       Refer to errata sheet of these devices for more details.
  * @{
  */

/* Peripherals on APB1 */
/**
  * @brief  TIM2 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM2()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM2_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM2()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM2_STOP)

/**
  * @brief  TIM3 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM3()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM3_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM3()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM3_STOP)

#if defined (DBGMCU_CR_DBG_TIM4_STOP)
/**
  * @brief  TIM4 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM4()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM4_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM4()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM4_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM5_STOP)
/**
  * @brief  TIM5 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM5()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM5_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM5()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM5_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM6_STOP)
/**
  * @brief  TIM6 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM6()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM6_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM6()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM6_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM7_STOP)
/**
  * @brief  TIM7 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM7()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM7_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM7()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM7_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM12_STOP)
/**
  * @brief  TIM12 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM12()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM12_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM12()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM12_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM13_STOP)
/**
  * @brief  TIM13 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM13()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM13_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM13()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM13_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM14_STOP)
/**
  * @brief  TIM14 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM14()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM14_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM14()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM14_STOP)
#endif

/**
  * @brief  WWDG Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_WWDG()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_WWDG_STOP)
#define __HAL_DBGMCU_UNFREEZE_WWDG()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_WWDG_STOP)

/**
  * @brief  IWDG Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_IWDG()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_IWDG_STOP)
#define __HAL_DBGMCU_UNFREEZE_IWDG()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_IWDG_STOP)

/**
  * @brief  I2C1 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_I2C1_TIMEOUT()    SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_I2C1_SMBUS_TIMEOUT)
#define __HAL_DBGMCU_UNFREEZE_I2C1_TIMEOUT()  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_I2C1_SMBUS_TIMEOUT)

#if defined (DBGMCU_CR_DBG_I2C2_SMBUS_TIMEOUT)
/**
  * @brief  I2C2 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_I2C2_TIMEOUT()    SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_I2C2_SMBUS_TIMEOUT)
#define __HAL_DBGMCU_UNFREEZE_I2C2_TIMEOUT()  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_I2C2_SMBUS_TIMEOUT)
#endif

#if defined (DBGMCU_CR_DBG_CAN1_STOP)
/**
  * @brief  CAN1 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_CAN1()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_CAN1_STOP)
#define __HAL_DBGMCU_UNFREEZE_CAN1()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_CAN1_STOP)
#endif

#if defined (DBGMCU_CR_DBG_CAN2_STOP)
/**
  * @brief  CAN2 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_CAN2()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_CAN2_STOP)
#define __HAL_DBGMCU_UNFREEZE_CAN2()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_CAN2_STOP)
#endif

/* Peripherals on APB2 */
#if defined (DBGMCU_CR_DBG_TIM1_STOP)
/**
  * @brief  TIM1 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM1()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM1_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM1()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM1_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM8_STOP)
/**
  * @brief  TIM8 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM8()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM8_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM8()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM8_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM9_STOP)
/**
  * @brief  TIM9 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM9()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM9_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM9()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM9_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM10_STOP)
/**
  * @brief  TIM10 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM10()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM10_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM10()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM10_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM11_STOP)
/**
  * @brief  TIM11 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM11()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM11_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM11()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM11_STOP)
#endif


#if defined (DBGMCU_CR_DBG_TIM15_STOP)
/**
  * @brief  TIM15 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM15()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM15_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM15()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM15_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM16_STOP)
/**
  * @brief  TIM16 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM16()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM16_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM16()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM16_STOP)
#endif

#if defined (DBGMCU_CR_DBG_TIM17_STOP)
/**
  * @brief  TIM17 Peripherals Debug mode
  */
#define __HAL_DBGMCU_FREEZE_TIM17()            SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM17_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM17()          CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TIM17_STOP)
#endif

/**
  * @}
  */

/** @defgroup HAL_Private_Macros HAL Private Macros
  * @{
  */
#define IS_TICKFREQ(FREQ) (((FREQ) == HAL_TICK_FREQ_10HZ)  || \
                           ((FREQ) == HAL_TICK_FREQ_100HZ) || \
                           ((FREQ) == HAL_TICK_FREQ_1KHZ))
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup HAL_Exported_Functions
  * @{
  */
/** @addtogroup HAL_Exported_Functions_Group1
  * @{
  */
/* Initialization and de-initialization functions  ******************************/
HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_DeInit(void);
void HAL_MspInit(void);
void HAL_MspDeInit(void);
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority);
/**
  * @}
  */

/** @addtogroup HAL_Exported_Functions_Group2
  * @{
  */
/* Peripheral Control functions  ************************************************/
void HAL_IncTick(void);
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
uint32_t HAL_GetTickPrio(void);
HAL_StatusTypeDef HAL_SetTickFreq(HAL_TickFreqTypeDef Freq);
HAL_TickFreqTypeDef HAL_GetTickFreq(void);
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);
uint32_t HAL_GetHalVersion(void);
uint32_t HAL_GetREVID(void);
uint32_t HAL_GetDEVID(void);
uint32_t HAL_GetUIDw0(void);
uint32_t HAL_GetUIDw1(void);
uint32_t HAL_GetUIDw2(void);
void HAL_DBGMCU_EnableDBGSleepMode(void);
void HAL_DBGMCU_DisableDBGSleepMode(void);
void HAL_DBGMCU_EnableDBGStopMode(void);
void HAL_DBGMCU_DisableDBGStopMode(void);
void HAL_DBGMCU_EnableDBGStandbyMode(void);
void HAL_DBGMCU_DisableDBGStandbyMode(void);
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @defgroup HAL_Private_Variables HAL Private Variables
  * @{
  */
/**
  * @}
  */
/* Private constants ---------------------------------------------------------*/
/** @defgroup HAL_Private_Constants HAL Private Constants
  * @{
  */
/**
  * @}
  */
/* Private macros ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/Legacy/stm32_hal_legacy.h`

```c

/* excerpt lines 1-60 */
/**
  ******************************************************************************
  * @file    stm32_hal_legacy.h
  * @author  MCD Application Team
  * @brief   This file contains aliases definition for the STM32Cube HAL constants
  *          macros and functions maintained for legacy purpose.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32_HAL_LEGACY
#define STM32_HAL_LEGACY

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** @defgroup HAL_AES_Aliased_Defines HAL CRYP Aliased Defines maintained for legacy purpose
  * @{
  */
#define AES_FLAG_RDERR                  CRYP_FLAG_RDERR
#define AES_FLAG_WRERR                  CRYP_FLAG_WRERR
#define AES_CLEARFLAG_CCF               CRYP_CLEARFLAG_CCF
#define AES_CLEARFLAG_RDERR             CRYP_CLEARFLAG_RDERR
#define AES_CLEARFLAG_WRERR             CRYP_CLEARFLAG_WRERR
#if defined(STM32H7) || defined(STM32MP1)
#define CRYP_DATATYPE_32B               CRYP_NO_SWAP
#define CRYP_DATATYPE_16B               CRYP_HALFWORD_SWAP
#define CRYP_DATATYPE_8B                CRYP_BYTE_SWAP
#define CRYP_DATATYPE_1B                CRYP_BIT_SWAP
#endif /* STM32H7 || STM32MP1 */
/**
  * @}
  */

/** @defgroup HAL_ADC_Aliased_Defines HAL ADC Aliased Defines maintained for legacy purpose
  * @{
  */
#define ADC_RESOLUTION12b               ADC_RESOLUTION_12B
#define ADC_RESOLUTION10b               ADC_RESOLUTION_10B
#define ADC_RESOLUTION8b                ADC_RESOLUTION_8B
#define ADC_RESOLUTION6b                ADC_RESOLUTION_6B
#define OVR_DATA_OVERWRITTEN            ADC_OVR_DATA_OVERWRITTEN
#define OVR_DATA_PRESERVED              ADC_OVR_DATA_PRESERVED
#define EOC_SINGLE_CONV                 ADC_EOC_SINGLE_CONV
#define EOC_SEQ_CONV                    ADC_EOC_SEQ_CONV

/* excerpt lines 281-347 */
#endif

#if defined(STM32U5)
#define DAC_TRIGGER_STOP_LPTIM1_OUT  DAC_TRIGGER_STOP_LPTIM1_CH1
#define DAC_TRIGGER_STOP_LPTIM3_OUT  DAC_TRIGGER_STOP_LPTIM3_CH1
#define DAC_TRIGGER_LPTIM1_OUT       DAC_TRIGGER_LPTIM1_CH1
#define DAC_TRIGGER_LPTIM3_OUT       DAC_TRIGGER_LPTIM3_CH1
#endif

#if defined(STM32H5)
#define DAC_TRIGGER_LPTIM1_OUT       DAC_TRIGGER_LPTIM1_CH1
#define DAC_TRIGGER_LPTIM2_OUT       DAC_TRIGGER_LPTIM2_CH1
#endif

#if defined(STM32L1) || defined(STM32L4) || defined(STM32G0) || defined(STM32L5) || defined(STM32H7) || \
    defined(STM32F4) || defined(STM32G4)
#define HAL_DAC_MSP_INIT_CB_ID       HAL_DAC_MSPINIT_CB_ID
#define HAL_DAC_MSP_DEINIT_CB_ID     HAL_DAC_MSPDEINIT_CB_ID
#endif

/**
  * @}
  */

/** @defgroup HAL_DMA_Aliased_Defines HAL DMA Aliased Defines maintained for legacy purpose
  * @{
  */
#define HAL_REMAPDMA_ADC_DMA_CH2                DMA_REMAP_ADC_DMA_CH2
#define HAL_REMAPDMA_USART1_TX_DMA_CH4          DMA_REMAP_USART1_TX_DMA_CH4
#define HAL_REMAPDMA_USART1_RX_DMA_CH5          DMA_REMAP_USART1_RX_DMA_CH5
#define HAL_REMAPDMA_TIM16_DMA_CH4              DMA_REMAP_TIM16_DMA_CH4
#define HAL_REMAPDMA_TIM17_DMA_CH2              DMA_REMAP_TIM17_DMA_CH2
#define HAL_REMAPDMA_USART3_DMA_CH32            DMA_REMAP_USART3_DMA_CH32
#define HAL_REMAPDMA_TIM16_DMA_CH6              DMA_REMAP_TIM16_DMA_CH6
#define HAL_REMAPDMA_TIM17_DMA_CH7              DMA_REMAP_TIM17_DMA_CH7
#define HAL_REMAPDMA_SPI2_DMA_CH67              DMA_REMAP_SPI2_DMA_CH67
#define HAL_REMAPDMA_USART2_DMA_CH67            DMA_REMAP_USART2_DMA_CH67
#define HAL_REMAPDMA_I2C1_DMA_CH76              DMA_REMAP_I2C1_DMA_CH76
#define HAL_REMAPDMA_TIM1_DMA_CH6               DMA_REMAP_TIM1_DMA_CH6
#define HAL_REMAPDMA_TIM2_DMA_CH7               DMA_REMAP_TIM2_DMA_CH7
#define HAL_REMAPDMA_TIM3_DMA_CH6               DMA_REMAP_TIM3_DMA_CH6

#define IS_HAL_REMAPDMA                          IS_DMA_REMAP
#define __HAL_REMAPDMA_CHANNEL_ENABLE            __HAL_DMA_REMAP_CHANNEL_ENABLE
#define __HAL_REMAPDMA_CHANNEL_DISABLE           __HAL_DMA_REMAP_CHANNEL_DISABLE

#if defined(STM32L4)

#define HAL_DMAMUX1_REQUEST_GEN_EXTI0            HAL_DMAMUX1_REQ_GEN_EXTI0
#define HAL_DMAMUX1_REQUEST_GEN_EXTI1            HAL_DMAMUX1_REQ_GEN_EXTI1
#define HAL_DMAMUX1_REQUEST_GEN_EXTI2            HAL_DMAMUX1_REQ_GEN_EXTI2
#define HAL_DMAMUX1_REQUEST_GEN_EXTI3            HAL_DMAMUX1_REQ_GEN_EXTI3
#define HAL_DMAMUX1_REQUEST_GEN_EXTI4            HAL_DMAMUX1_REQ_GEN_EXTI4
#define HAL_DMAMUX1_REQUEST_GEN_EXTI5            HAL_DMAMUX1_REQ_GEN_EXTI5
#define HAL_DMAMUX1_REQUEST_GEN_EXTI6            HAL_DMAMUX1_REQ_GEN_EXTI6
#define HAL_DMAMUX1_REQUEST_GEN_EXTI7            HAL_DMAMUX1_REQ_GEN_EXTI7
#define HAL_DMAMUX1_REQUEST_GEN_EXTI8            HAL_DMAMUX1_REQ_GEN_EXTI8
#define HAL_DMAMUX1_REQUEST_GEN_EXTI9            HAL_DMAMUX1_REQ_GEN_EXTI9
#define HAL_DMAMUX1_REQUEST_GEN_EXTI10           HAL_DMAMUX1_REQ_GEN_EXTI10
#define HAL_DMAMUX1_REQUEST_GEN_EXTI11           HAL_DMAMUX1_REQ_GEN_EXTI11
#define HAL_DMAMUX1_REQUEST_GEN_EXTI12           HAL_DMAMUX1_REQ_GEN_EXTI12
#define HAL_DMAMUX1_REQUEST_GEN_EXTI13           HAL_DMAMUX1_REQ_GEN_EXTI13
#define HAL_DMAMUX1_REQUEST_GEN_EXTI14           HAL_DMAMUX1_REQ_GEN_EXTI14
#define HAL_DMAMUX1_REQUEST_GEN_EXTI15           HAL_DMAMUX1_REQ_GEN_EXTI15
#define HAL_DMAMUX1_REQUEST_GEN_DMAMUX1_CH0_EVT  HAL_DMAMUX1_REQ_GEN_DMAMUX1_CH0_EVT
#define HAL_DMAMUX1_REQUEST_GEN_DMAMUX1_CH1_EVT  HAL_DMAMUX1_REQ_GEN_DMAMUX1_CH1_EVT
#define HAL_DMAMUX1_REQUEST_GEN_DMAMUX1_CH2_EVT  HAL_DMAMUX1_REQ_GEN_DMAMUX1_CH2_EVT

/* excerpt lines 395-449 */
#define HAL_DMAMUX2_REQUEST_GEN_DMAMUX2_CH0_EVT    HAL_DMAMUX2_REQ_GEN_DMAMUX2_CH0_EVT
#define HAL_DMAMUX2_REQUEST_GEN_DMAMUX2_CH1_EVT    HAL_DMAMUX2_REQ_GEN_DMAMUX2_CH1_EVT
#define HAL_DMAMUX2_REQUEST_GEN_DMAMUX2_CH2_EVT    HAL_DMAMUX2_REQ_GEN_DMAMUX2_CH2_EVT
#define HAL_DMAMUX2_REQUEST_GEN_DMAMUX2_CH3_EVT    HAL_DMAMUX2_REQ_GEN_DMAMUX2_CH3_EVT
#define HAL_DMAMUX2_REQUEST_GEN_DMAMUX2_CH4_EVT    HAL_DMAMUX2_REQ_GEN_DMAMUX2_CH4_EVT
#define HAL_DMAMUX2_REQUEST_GEN_DMAMUX2_CH5_EVT    HAL_DMAMUX2_REQ_GEN_DMAMUX2_CH5_EVT
#define HAL_DMAMUX2_REQUEST_GEN_DMAMUX2_CH6_EVT    HAL_DMAMUX2_REQ_GEN_DMAMUX2_CH6_EVT
#define HAL_DMAMUX2_REQUEST_GEN_LPUART1_RX_WKUP    HAL_DMAMUX2_REQ_GEN_LPUART1_RX_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_LPUART1_TX_WKUP    HAL_DMAMUX2_REQ_GEN_LPUART1_TX_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_LPTIM2_WKUP        HAL_DMAMUX2_REQ_GEN_LPTIM2_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_LPTIM2_OUT         HAL_DMAMUX2_REQ_GEN_LPTIM2_OUT
#define HAL_DMAMUX2_REQUEST_GEN_LPTIM3_WKUP        HAL_DMAMUX2_REQ_GEN_LPTIM3_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_LPTIM3_OUT         HAL_DMAMUX2_REQ_GEN_LPTIM3_OUT
#define HAL_DMAMUX2_REQUEST_GEN_LPTIM4_WKUP        HAL_DMAMUX2_REQ_GEN_LPTIM4_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_LPTIM5_WKUP        HAL_DMAMUX2_REQ_GEN_LPTIM5_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_I2C4_WKUP          HAL_DMAMUX2_REQ_GEN_I2C4_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_SPI6_WKUP          HAL_DMAMUX2_REQ_GEN_SPI6_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_COMP1_OUT          HAL_DMAMUX2_REQ_GEN_COMP1_OUT
#define HAL_DMAMUX2_REQUEST_GEN_COMP2_OUT          HAL_DMAMUX2_REQ_GEN_COMP2_OUT
#define HAL_DMAMUX2_REQUEST_GEN_RTC_WKUP           HAL_DMAMUX2_REQ_GEN_RTC_WKUP
#define HAL_DMAMUX2_REQUEST_GEN_EXTI0              HAL_DMAMUX2_REQ_GEN_EXTI0
#define HAL_DMAMUX2_REQUEST_GEN_EXTI2              HAL_DMAMUX2_REQ_GEN_EXTI2
#define HAL_DMAMUX2_REQUEST_GEN_I2C4_IT_EVT        HAL_DMAMUX2_REQ_GEN_I2C4_IT_EVT
#define HAL_DMAMUX2_REQUEST_GEN_SPI6_IT            HAL_DMAMUX2_REQ_GEN_SPI6_IT
#define HAL_DMAMUX2_REQUEST_GEN_LPUART1_TX_IT      HAL_DMAMUX2_REQ_GEN_LPUART1_TX_IT
#define HAL_DMAMUX2_REQUEST_GEN_LPUART1_RX_IT      HAL_DMAMUX2_REQ_GEN_LPUART1_RX_IT
#define HAL_DMAMUX2_REQUEST_GEN_ADC3_IT            HAL_DMAMUX2_REQ_GEN_ADC3_IT
#define HAL_DMAMUX2_REQUEST_GEN_ADC3_AWD1_OUT      HAL_DMAMUX2_REQ_GEN_ADC3_AWD1_OUT
#define HAL_DMAMUX2_REQUEST_GEN_BDMA_CH0_IT        HAL_DMAMUX2_REQ_GEN_BDMA_CH0_IT
#define HAL_DMAMUX2_REQUEST_GEN_BDMA_CH1_IT        HAL_DMAMUX2_REQ_GEN_BDMA_CH1_IT

#define HAL_DMAMUX_REQUEST_GEN_NO_EVENT            HAL_DMAMUX_REQ_GEN_NO_EVENT
#define HAL_DMAMUX_REQUEST_GEN_RISING              HAL_DMAMUX_REQ_GEN_RISING
#define HAL_DMAMUX_REQUEST_GEN_FALLING             HAL_DMAMUX_REQ_GEN_FALLING
#define HAL_DMAMUX_REQUEST_GEN_RISING_FALLING      HAL_DMAMUX_REQ_GEN_RISING_FALLING

#define DFSDM_FILTER_EXT_TRIG_LPTIM1               DFSDM_FILTER_EXT_TRIG_LPTIM1_OUT
#define DFSDM_FILTER_EXT_TRIG_LPTIM2               DFSDM_FILTER_EXT_TRIG_LPTIM2_OUT
#define DFSDM_FILTER_EXT_TRIG_LPTIM3               DFSDM_FILTER_EXT_TRIG_LPTIM3_OUT

#define DAC_TRIGGER_LP1_OUT                        DAC_TRIGGER_LPTIM1_OUT
#define DAC_TRIGGER_LP2_OUT                        DAC_TRIGGER_LPTIM2_OUT

#endif /* STM32H7 */

#if defined(STM32U5)
#define GPDMA1_REQUEST_DCMI                        GPDMA1_REQUEST_DCMI_PSSI
#endif /* STM32U5 */
/**
  * @}
  */

/** @defgroup HAL_FLASH_Aliased_Defines HAL FLASH Aliased Defines maintained for legacy purpose
  * @{
  */

/* excerpt lines 1158-1205 */
#define OPAMP_INVERTINGINPUT_VM0              OPAMP_INVERTINGINPUT_IO0
#define OPAMP_INVERTINGINPUT_VM1              OPAMP_INVERTINGINPUT_IO1

#define IOPAMP_INVERTINGINPUT_VM0             OPAMP_INVERTINGINPUT_IO0
#define IOPAMP_INVERTINGINPUT_VM1             OPAMP_INVERTINGINPUT_IO1

#define OPAMP_SEC_INVERTINGINPUT_VM0          OPAMP_SEC_INVERTINGINPUT_IO0
#define OPAMP_SEC_INVERTINGINPUT_VM1          OPAMP_SEC_INVERTINGINPUT_IO1

#define OPAMP_INVERTINGINPUT_VINM             OPAMP_SEC_INVERTINGINPUT_IO1

#define OPAMP_PGACONNECT_NO                   OPAMP_PGA_CONNECT_INVERTINGINPUT_NO
#define OPAMP_PGACONNECT_VM0                  OPAMP_PGA_CONNECT_INVERTINGINPUT_IO0
#define OPAMP_PGACONNECT_VM1                  OPAMP_PGA_CONNECT_INVERTINGINPUT_IO1

#if defined(STM32L1) || defined(STM32L4) || defined(STM32L5) || defined(STM32H7) || defined(STM32G4) || defined(STM32U5)
#define HAL_OPAMP_MSP_INIT_CB_ID       HAL_OPAMP_MSPINIT_CB_ID
#define HAL_OPAMP_MSP_DEINIT_CB_ID     HAL_OPAMP_MSPDEINIT_CB_ID
#endif

#if defined(STM32L4) || defined(STM32L5)
#define OPAMP_POWERMODE_NORMAL                OPAMP_POWERMODE_NORMALPOWER
#elif defined(STM32G4)
#define OPAMP_POWERMODE_NORMAL                OPAMP_POWERMODE_NORMALSPEED
#endif

/**
  * @}
  */

/** @defgroup HAL_I2S_Aliased_Defines HAL I2S Aliased Defines maintained for legacy purpose
  * @{
  */
#define I2S_STANDARD_PHILLIPS      I2S_STANDARD_PHILIPS

#if defined(STM32H7)
#define I2S_IT_TXE               I2S_IT_TXP
#define I2S_IT_RXNE              I2S_IT_RXP

#define I2S_FLAG_TXE             I2S_FLAG_TXP
#define I2S_FLAG_RXNE            I2S_FLAG_RXP
#endif

#if defined(STM32F7)
#define I2S_CLOCK_SYSCLK           I2S_CLOCK_PLL
#endif
/**
  * @}

/* excerpt lines 1334-1404 */
  */
#define SMBUS_DUALADDRESS_DISABLED      SMBUS_DUALADDRESS_DISABLE
#define SMBUS_DUALADDRESS_ENABLED       SMBUS_DUALADDRESS_ENABLE
#define SMBUS_GENERALCALL_DISABLED      SMBUS_GENERALCALL_DISABLE
#define SMBUS_GENERALCALL_ENABLED       SMBUS_GENERALCALL_ENABLE
#define SMBUS_NOSTRETCH_DISABLED        SMBUS_NOSTRETCH_DISABLE
#define SMBUS_NOSTRETCH_ENABLED         SMBUS_NOSTRETCH_ENABLE
#define SMBUS_ANALOGFILTER_ENABLED      SMBUS_ANALOGFILTER_ENABLE
#define SMBUS_ANALOGFILTER_DISABLED     SMBUS_ANALOGFILTER_DISABLE
#define SMBUS_PEC_DISABLED              SMBUS_PEC_DISABLE
#define SMBUS_PEC_ENABLED               SMBUS_PEC_ENABLE
#define HAL_SMBUS_STATE_SLAVE_LISTEN    HAL_SMBUS_STATE_LISTEN
/**
  * @}
  */

/** @defgroup HAL_SPI_Aliased_Defines HAL SPI Aliased Defines maintained for legacy purpose
  * @{
  */
#define SPI_TIMODE_DISABLED             SPI_TIMODE_DISABLE
#define SPI_TIMODE_ENABLED              SPI_TIMODE_ENABLE

#define SPI_CRCCALCULATION_DISABLED     SPI_CRCCALCULATION_DISABLE
#define SPI_CRCCALCULATION_ENABLED      SPI_CRCCALCULATION_ENABLE

#define SPI_NSS_PULSE_DISABLED          SPI_NSS_PULSE_DISABLE
#define SPI_NSS_PULSE_ENABLED           SPI_NSS_PULSE_ENABLE

#if defined(STM32H7)

#define SPI_FLAG_TXE                    SPI_FLAG_TXP
#define SPI_FLAG_RXNE                   SPI_FLAG_RXP

#define SPI_IT_TXE                      SPI_IT_TXP
#define SPI_IT_RXNE                     SPI_IT_RXP

#define SPI_FRLVL_EMPTY                 SPI_RX_FIFO_0PACKET
#define SPI_FRLVL_QUARTER_FULL          SPI_RX_FIFO_1PACKET
#define SPI_FRLVL_HALF_FULL             SPI_RX_FIFO_2PACKET
#define SPI_FRLVL_FULL                  SPI_RX_FIFO_3PACKET

#endif /* STM32H7 */

/**
  * @}
  */

/** @defgroup HAL_TIM_Aliased_Defines HAL TIM Aliased Defines maintained for legacy purpose
  * @{
  */
#define CCER_CCxE_MASK                   TIM_CCER_CCxE_MASK
#define CCER_CCxNE_MASK                  TIM_CCER_CCxNE_MASK

#define TIM_DMABase_CR1                  TIM_DMABASE_CR1
#define TIM_DMABase_CR2                  TIM_DMABASE_CR2
#define TIM_DMABase_SMCR                 TIM_DMABASE_SMCR
#define TIM_DMABase_DIER                 TIM_DMABASE_DIER
#define TIM_DMABase_SR                   TIM_DMABASE_SR
#define TIM_DMABase_EGR                  TIM_DMABASE_EGR
#define TIM_DMABase_CCMR1                TIM_DMABASE_CCMR1
#define TIM_DMABase_CCMR2                TIM_DMABASE_CCMR2
#define TIM_DMABase_CCER                 TIM_DMABASE_CCER
#define TIM_DMABase_CNT                  TIM_DMABASE_CNT
#define TIM_DMABase_PSC                  TIM_DMABASE_PSC
#define TIM_DMABase_ARR                  TIM_DMABASE_ARR
#define TIM_DMABase_RCR                  TIM_DMABASE_RCR
#define TIM_DMABase_CCR1                 TIM_DMABASE_CCR1
#define TIM_DMABase_CCR2                 TIM_DMABASE_CCR2
#define TIM_DMABase_CCR3                 TIM_DMABASE_CCR3
#define TIM_DMABase_CCR4                 TIM_DMABASE_CCR4
#define TIM_DMABase_BDTR                 TIM_DMABASE_BDTR

/* excerpt lines 2024-2074 */
#endif /* STM32H5 || STM32WBA || STM32H7RS */

/**
  * @}
  */

/** @defgroup HAL_SMBUS_Aliased_Functions HAL SMBUS Aliased Functions maintained for legacy purpose
  * @{
  */
#define HAL_SMBUS_Slave_Listen_IT          HAL_SMBUS_EnableListen_IT
#define HAL_SMBUS_SlaveAddrCallback        HAL_SMBUS_AddrCallback
#define HAL_SMBUS_SlaveListenCpltCallback  HAL_SMBUS_ListenCpltCallback
/**
  * @}
  */

/** @defgroup HAL_SPI_Aliased_Functions HAL SPI Aliased Functions maintained for legacy purpose
  * @{
  */
#define HAL_SPI_FlushRxFifo                HAL_SPIEx_FlushRxFifo
/**
  * @}
  */

/** @defgroup HAL_TIM_Aliased_Functions HAL TIM Aliased Functions maintained for legacy purpose
  * @{
  */
#define HAL_TIM_DMADelayPulseCplt                       TIM_DMADelayPulseCplt
#define HAL_TIM_DMAError                                TIM_DMAError
#define HAL_TIM_DMACaptureCplt                          TIM_DMACaptureCplt
#define HAL_TIMEx_DMACommutationCplt                    TIMEx_DMACommutationCplt
#if defined(STM32H7) || defined(STM32G0) || defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || \
    defined(STM32F3) || defined(STM32F4) || defined(STM32F7) || defined(STM32L0) || defined(STM32L4)
#define HAL_TIM_SlaveConfigSynchronization              HAL_TIM_SlaveConfigSynchro
#define HAL_TIM_SlaveConfigSynchronization_IT           HAL_TIM_SlaveConfigSynchro_IT
#define HAL_TIMEx_CommutationCallback                   HAL_TIMEx_CommutCallback
#define HAL_TIMEx_ConfigCommutationEvent                HAL_TIMEx_ConfigCommutEvent
#define HAL_TIMEx_ConfigCommutationEvent_IT             HAL_TIMEx_ConfigCommutEvent_IT
#define HAL_TIMEx_ConfigCommutationEvent_DMA            HAL_TIMEx_ConfigCommutEvent_DMA
#endif /* STM32H7 || STM32G0 || STM32F0 || STM32F1 || STM32F2 || STM32F3 || STM32F4 || STM32F7 || STM32L0 */
/**
  * @}
  */

/** @defgroup HAL_UART_Aliased_Functions HAL UART Aliased Functions maintained for legacy purpose
  * @{
  */
#define HAL_UART_WakeupCallback HAL_UARTEx_WakeupCallback
/**
  * @}
  */

/* excerpt lines 2102-2149 */
#define AES_IT_ERR                     CRYP_IT_ERR
#define AES_FLAG_CCF                   CRYP_FLAG_CCF
/**
  * @}
  */

/** @defgroup HAL_Aliased_Macros HAL Generic Aliased Macros maintained for legacy purpose
  * @{
  */
#define __HAL_GET_BOOT_MODE                   __HAL_SYSCFG_GET_BOOT_MODE
#define __HAL_REMAPMEMORY_FLASH               __HAL_SYSCFG_REMAPMEMORY_FLASH
#define __HAL_REMAPMEMORY_SYSTEMFLASH         __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH
#define __HAL_REMAPMEMORY_SRAM                __HAL_SYSCFG_REMAPMEMORY_SRAM
#define __HAL_REMAPMEMORY_FMC                 __HAL_SYSCFG_REMAPMEMORY_FMC
#define __HAL_REMAPMEMORY_FMC_SDRAM           __HAL_SYSCFG_REMAPMEMORY_FMC_SDRAM
#define __HAL_REMAPMEMORY_FSMC                __HAL_SYSCFG_REMAPMEMORY_FSMC
#define __HAL_REMAPMEMORY_QUADSPI             __HAL_SYSCFG_REMAPMEMORY_QUADSPI
#define __HAL_FMC_BANK                        __HAL_SYSCFG_FMC_BANK
#define __HAL_GET_FLAG                        __HAL_SYSCFG_GET_FLAG
#define __HAL_CLEAR_FLAG                      __HAL_SYSCFG_CLEAR_FLAG
#define __HAL_VREFINT_OUT_ENABLE              __HAL_SYSCFG_VREFINT_OUT_ENABLE
#define __HAL_VREFINT_OUT_DISABLE             __HAL_SYSCFG_VREFINT_OUT_DISABLE
#define __HAL_SYSCFG_SRAM2_WRP_ENABLE         __HAL_SYSCFG_SRAM2_WRP_0_31_ENABLE

#define SYSCFG_FLAG_VREF_READY                SYSCFG_FLAG_VREFINT_READY
#define SYSCFG_FLAG_RC48                      RCC_FLAG_HSI48
#define IS_SYSCFG_FASTMODEPLUS_CONFIG         IS_I2C_FASTMODEPLUS
#define UFB_MODE_BitNumber                    UFB_MODE_BIT_NUMBER
#define CMP_PD_BitNumber                      CMP_PD_BIT_NUMBER

/**
  * @}
  */


/** @defgroup HAL_ADC_Aliased_Macros HAL ADC Aliased Macros maintained for legacy purpose
  * @{
  */
#define __ADC_ENABLE                                     __HAL_ADC_ENABLE
#define __ADC_DISABLE                                    __HAL_ADC_DISABLE
#define __HAL_ADC_ENABLING_CONDITIONS                    ADC_ENABLING_CONDITIONS
#define __HAL_ADC_DISABLING_CONDITIONS                   ADC_DISABLING_CONDITIONS
#define __HAL_ADC_IS_ENABLED                             ADC_IS_ENABLE
#define __ADC_IS_ENABLED                                 ADC_IS_ENABLE
#define __HAL_ADC_IS_SOFTWARE_START_REGULAR              ADC_IS_SOFTWARE_START_REGULAR
#define __HAL_ADC_IS_SOFTWARE_START_INJECTED             ADC_IS_SOFTWARE_START_INJECTED
#define __HAL_ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED ADC_IS_CONVERSION_ONGOING_REGULAR_INJECTED
#define __HAL_ADC_IS_CONVERSION_ONGOING_REGULAR          ADC_IS_CONVERSION_ONGOING_REGULAR

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/stm32f1xx_hal_rcc.h`

```c

/* excerpt lines 1-60 */
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_rcc.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1xx_HAL_RCC_H
#define __STM32F1xx_HAL_RCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal_def.h"


/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @addtogroup RCC
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup RCC_Exported_Types RCC Exported Types
  * @{
  */

/**
  * @brief  RCC PLL configuration structure definition
  */
typedef struct
{
  uint32_t PLLState;      /*!< PLLState: The new state of the PLL.
                              This parameter can be a value of @ref RCC_PLL_Config */

  uint32_t PLLSource;     /*!< PLLSource: PLL entry clock source.
                              This parameter must be a value of @ref RCC_PLL_Clock_Source */

  uint32_t PLLMUL;        /*!< PLLMUL: Multiplication factor for PLL VCO input clock
                              This parameter must be a value of @ref RCCEx_PLL_Multiplication_Factor */
} RCC_PLLInitTypeDef;


/* excerpt lines 535-636 */
#define __HAL_RCC_ADC1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_TIM1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_SPI1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_USART1_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg; \
                                        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);\
                                        /* Delay after an RCC peripheral clock enabling */\
                                        tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);\
                                        UNUSED(tmpreg); \
                                      } while(0U)

#define __HAL_RCC_AFIO_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_AFIOEN))
#define __HAL_RCC_GPIOA_CLK_DISABLE()     (RCC->APB2ENR &= ~(RCC_APB2ENR_IOPAEN))
#define __HAL_RCC_GPIOB_CLK_DISABLE()     (RCC->APB2ENR &= ~(RCC_APB2ENR_IOPBEN))
#define __HAL_RCC_GPIOC_CLK_DISABLE()     (RCC->APB2ENR &= ~(RCC_APB2ENR_IOPCEN))
#define __HAL_RCC_GPIOD_CLK_DISABLE()     (RCC->APB2ENR &= ~(RCC_APB2ENR_IOPDEN))
#define __HAL_RCC_ADC1_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_ADC1EN))

#define __HAL_RCC_TIM1_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_TIM1EN))
#define __HAL_RCC_SPI1_CLK_DISABLE()      (RCC->APB2ENR &= ~(RCC_APB2ENR_SPI1EN))
#define __HAL_RCC_USART1_CLK_DISABLE()    (RCC->APB2ENR &= ~(RCC_APB2ENR_USART1EN))

/**
  * @}
  */

/** @defgroup RCC_APB2_Peripheral_Clock_Enable_Disable_Status APB2 Peripheral Clock Enable Disable Status
  * @brief  Get the enable or disable status of the APB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it.
  * @{
  */

#define __HAL_RCC_AFIO_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_AFIOEN)) != RESET)
#define __HAL_RCC_AFIO_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_AFIOEN)) == RESET)
#define __HAL_RCC_GPIOA_IS_CLK_ENABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_IOPAEN)) != RESET)
#define __HAL_RCC_GPIOA_IS_CLK_DISABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_IOPAEN)) == RESET)
#define __HAL_RCC_GPIOB_IS_CLK_ENABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_IOPBEN)) != RESET)
#define __HAL_RCC_GPIOB_IS_CLK_DISABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_IOPBEN)) == RESET)
#define __HAL_RCC_GPIOC_IS_CLK_ENABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_IOPCEN)) != RESET)
#define __HAL_RCC_GPIOC_IS_CLK_DISABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_IOPCEN)) == RESET)
#define __HAL_RCC_GPIOD_IS_CLK_ENABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_IOPDEN)) != RESET)
#define __HAL_RCC_GPIOD_IS_CLK_DISABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_IOPDEN)) == RESET)
#define __HAL_RCC_ADC1_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_ADC1EN)) != RESET)
#define __HAL_RCC_ADC1_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_ADC1EN)) == RESET)
#define __HAL_RCC_TIM1_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_TIM1EN)) != RESET)
#define __HAL_RCC_TIM1_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_TIM1EN)) == RESET)
#define __HAL_RCC_SPI1_IS_CLK_ENABLED()       ((RCC->APB2ENR & (RCC_APB2ENR_SPI1EN)) != RESET)
#define __HAL_RCC_SPI1_IS_CLK_DISABLED()      ((RCC->APB2ENR & (RCC_APB2ENR_SPI1EN)) == RESET)
#define __HAL_RCC_USART1_IS_CLK_ENABLED()     ((RCC->APB2ENR & (RCC_APB2ENR_USART1EN)) != RESET)
#define __HAL_RCC_USART1_IS_CLK_DISABLED()    ((RCC->APB2ENR & (RCC_APB2ENR_USART1EN)) == RESET)

/**
  * @}
  */

/** @defgroup RCC_APB1_Force_Release_Reset APB1 Force Release Reset
  * @brief  Force or release APB1 peripheral reset.
  * @{
  */
#define __HAL_RCC_APB1_FORCE_RESET()       (RCC->APB2RSTR = 0xFFFFFFFFU)
#define __HAL_RCC_TIM2_FORCE_RESET()       (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_TIM3_FORCE_RESET()       (RCC->APB1RSTR |= (RCC_APB1RSTR_TIM3RST))
#define __HAL_RCC_WWDG_FORCE_RESET()       (RCC->APB1RSTR |= (RCC_APB1RSTR_WWDGRST))
#define __HAL_RCC_USART2_FORCE_RESET()     (RCC->APB1RSTR |= (RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_I2C1_FORCE_RESET()       (RCC->APB1RSTR |= (RCC_APB1RSTR_I2C1RST))

#define __HAL_RCC_BKP_FORCE_RESET()        (RCC->APB1RSTR |= (RCC_APB1RSTR_BKPRST))
#define __HAL_RCC_PWR_FORCE_RESET()        (RCC->APB1RSTR |= (RCC_APB1RSTR_PWRRST))

#define __HAL_RCC_APB1_RELEASE_RESET()      (RCC->APB1RSTR = 0x00)
#define __HAL_RCC_TIM2_RELEASE_RESET()       (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST))
#define __HAL_RCC_TIM3_RELEASE_RESET()       (RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM3RST))
#define __HAL_RCC_WWDG_RELEASE_RESET()       (RCC->APB1RSTR &= ~(RCC_APB1RSTR_WWDGRST))
#define __HAL_RCC_USART2_RELEASE_RESET()     (RCC->APB1RSTR &= ~(RCC_APB1RSTR_USART2RST))
#define __HAL_RCC_I2C1_RELEASE_RESET()       (RCC->APB1RSTR &= ~(RCC_APB1RSTR_I2C1RST))

#define __HAL_RCC_BKP_RELEASE_RESET()        (RCC->APB1RSTR &= ~(RCC_APB1RSTR_BKPRST))
#define __HAL_RCC_PWR_RELEASE_RESET()        (RCC->APB1RSTR &= ~(RCC_APB1RSTR_PWRRST))


/* excerpt lines 638-697 */
  * @}
  */

/** @defgroup RCC_APB2_Force_Release_Reset APB2 Force Release Reset
  * @brief  Force or release APB2 peripheral reset.
  * @{
  */
#define __HAL_RCC_APB2_FORCE_RESET()       (RCC->APB2RSTR = 0xFFFFFFFFU)
#define __HAL_RCC_AFIO_FORCE_RESET()       (RCC->APB2RSTR |= (RCC_APB2RSTR_AFIORST))
#define __HAL_RCC_GPIOA_FORCE_RESET()      (RCC->APB2RSTR |= (RCC_APB2RSTR_IOPARST))
#define __HAL_RCC_GPIOB_FORCE_RESET()      (RCC->APB2RSTR |= (RCC_APB2RSTR_IOPBRST))
#define __HAL_RCC_GPIOC_FORCE_RESET()      (RCC->APB2RSTR |= (RCC_APB2RSTR_IOPCRST))
#define __HAL_RCC_GPIOD_FORCE_RESET()      (RCC->APB2RSTR |= (RCC_APB2RSTR_IOPDRST))
#define __HAL_RCC_ADC1_FORCE_RESET()       (RCC->APB2RSTR |= (RCC_APB2RSTR_ADC1RST))

#define __HAL_RCC_TIM1_FORCE_RESET()       (RCC->APB2RSTR |= (RCC_APB2RSTR_TIM1RST))
#define __HAL_RCC_SPI1_FORCE_RESET()       (RCC->APB2RSTR |= (RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_USART1_FORCE_RESET()     (RCC->APB2RSTR |= (RCC_APB2RSTR_USART1RST))

#define __HAL_RCC_APB2_RELEASE_RESET()      (RCC->APB2RSTR = 0x00)
#define __HAL_RCC_AFIO_RELEASE_RESET()       (RCC->APB2RSTR &= ~(RCC_APB2RSTR_AFIORST))
#define __HAL_RCC_GPIOA_RELEASE_RESET()      (RCC->APB2RSTR &= ~(RCC_APB2RSTR_IOPARST))
#define __HAL_RCC_GPIOB_RELEASE_RESET()      (RCC->APB2RSTR &= ~(RCC_APB2RSTR_IOPBRST))
#define __HAL_RCC_GPIOC_RELEASE_RESET()      (RCC->APB2RSTR &= ~(RCC_APB2RSTR_IOPCRST))
#define __HAL_RCC_GPIOD_RELEASE_RESET()      (RCC->APB2RSTR &= ~(RCC_APB2RSTR_IOPDRST))
#define __HAL_RCC_ADC1_RELEASE_RESET()       (RCC->APB2RSTR &= ~(RCC_APB2RSTR_ADC1RST))

#define __HAL_RCC_TIM1_RELEASE_RESET()       (RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM1RST))
#define __HAL_RCC_SPI1_RELEASE_RESET()       (RCC->APB2RSTR &= ~(RCC_APB2RSTR_SPI1RST))
#define __HAL_RCC_USART1_RELEASE_RESET()     (RCC->APB2RSTR &= ~(RCC_APB2RSTR_USART1RST))

/**
  * @}
  */

/** @defgroup RCC_HSI_Configuration HSI Configuration
  * @{
  */

/** @brief  Macros to enable or disable the Internal High Speed oscillator (HSI).
  * @note   The HSI is stopped by hardware when entering STOP and STANDBY modes.
  * @note   HSI can not be stopped if it is used as system clock source. In this case,
  *         you have to select another source of the system clock then stop the HSI.
  * @note   After enabling the HSI, the application software should wait on HSIRDY
  *         flag to be set indicating that HSI clock is stable and can be used as
  *         system clock source.
  * @note   When the HSI is stopped, HSIRDY flag goes low after 6 HSI oscillator
  *         clock cycles.
  */
#define __HAL_RCC_HSI_ENABLE()  (*(__IO uint32_t *) RCC_CR_HSION_BB = ENABLE)
#define __HAL_RCC_HSI_DISABLE() (*(__IO uint32_t *) RCC_CR_HSION_BB = DISABLE)

/** @brief  Macro to adjust the Internal High Speed oscillator (HSI) calibration value.
  * @note   The calibration is used to compensate for the variations in voltage
  *         and temperature that influence the frequency of the internal HSI RC.
  * @param  _HSICALIBRATIONVALUE_ specifies the calibration trimming value.
  *         (default is RCC_HSICALIBRATION_DEFAULT).
  *         This parameter must be a number between 0 and 0x1F.
  */
#define __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(_HSICALIBRATIONVALUE_) \

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/stm32f1xx_hal_cortex.h`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_cortex.h
  * @author  MCD Application Team
  * @brief   Header file of CORTEX HAL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1xx_HAL_CORTEX_H
#define __STM32F1xx_HAL_CORTEX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal_def.h"

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @addtogroup CORTEX
  * @{
  */ 
/* Exported types ------------------------------------------------------------*/
/** @defgroup CORTEX_Exported_Types Cortex Exported Types
  * @{
  */

#if (__MPU_PRESENT == 1U)
/** @defgroup CORTEX_MPU_Region_Initialization_Structure_definition MPU Region Initialization Structure Definition
  * @brief  MPU Region initialization structure 
  * @{
  */
typedef struct
{
  uint8_t                Enable;                /*!< Specifies the status of the region. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Region_Enable                 */
  uint8_t                Number;                /*!< Specifies the number of the region to protect. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Region_Number                 */
  uint32_t               BaseAddress;           /*!< Specifies the base address of the region to protect.                           */
  uint8_t                Size;                  /*!< Specifies the size of the region to protect. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Region_Size                   */
  uint8_t                SubRegionDisable;      /*!< Specifies the number of the subregion protection to disable. 
                                                     This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF    */         
  uint8_t                TypeExtField;          /*!< Specifies the TEX field level.
                                                     This parameter can be a value of @ref CORTEX_MPU_TEX_Levels                    */                 
  uint8_t                AccessPermission;      /*!< Specifies the region access permission type. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Region_Permission_Attributes  */
  uint8_t                DisableExec;           /*!< Specifies the instruction access status. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Instruction_Access            */
  uint8_t                IsShareable;           /*!< Specifies the shareability status of the protected region. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Access_Shareable              */
  uint8_t                IsCacheable;           /*!< Specifies the cacheable status of the region protected. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Access_Cacheable              */
  uint8_t                IsBufferable;          /*!< Specifies the bufferable status of the protected region. 
                                                     This parameter can be a value of @ref CORTEX_MPU_Access_Bufferable             */
}MPU_Region_InitTypeDef;
/**
  * @}
  */
#endif /* __MPU_PRESENT */

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup CORTEX_Exported_Constants CORTEX Exported Constants
  * @{
  */

/** @defgroup CORTEX_Preemption_Priority_Group CORTEX Preemption Priority Group
  * @{
  */
#define NVIC_PRIORITYGROUP_0         0x00000007U /*!< 0 bits for pre-emption priority
                                                      4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         0x00000006U /*!< 1 bits for pre-emption priority
                                                      3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         0x00000005U /*!< 2 bits for pre-emption priority
                                                      2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         0x00000004U /*!< 3 bits for pre-emption priority
                                                      1 bits for subpriority */
#define NVIC_PRIORITYGROUP_4         0x00000003U /*!< 4 bits for pre-emption priority
                                                      0 bits for subpriority */
/**
  * @}
  */

/** @defgroup CORTEX_SysTick_clock_source CORTEX _SysTick clock source 
  * @{
  */
#define SYSTICK_CLKSOURCE_HCLK_DIV8    0x00000000U
#define SYSTICK_CLKSOURCE_HCLK         0x00000004U

/**
  * @}
  */

#if (__MPU_PRESENT == 1)
/** @defgroup CORTEX_MPU_HFNMI_PRIVDEF_Control MPU HFNMI and PRIVILEGED Access control
  * @{
  */
#define  MPU_HFNMI_PRIVDEF_NONE           0x00000000U
#define  MPU_HARDFAULT_NMI                MPU_CTRL_HFNMIENA_Msk
#define  MPU_PRIVILEGED_DEFAULT           MPU_CTRL_PRIVDEFENA_Msk
#define  MPU_HFNMI_PRIVDEF               (MPU_CTRL_HFNMIENA_Msk | MPU_CTRL_PRIVDEFENA_Msk)

/**
  * @}
  */

/** @defgroup CORTEX_MPU_Region_Enable CORTEX MPU Region Enable
  * @{
  */
#define  MPU_REGION_ENABLE     ((uint8_t)0x01)
#define  MPU_REGION_DISABLE    ((uint8_t)0x00)
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Instruction_Access CORTEX MPU Instruction Access
  * @{
  */
#define  MPU_INSTRUCTION_ACCESS_ENABLE      ((uint8_t)0x00)
#define  MPU_INSTRUCTION_ACCESS_DISABLE     ((uint8_t)0x01)
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Access_Shareable CORTEX MPU Instruction Access Shareable
  * @{
  */
#define  MPU_ACCESS_SHAREABLE        ((uint8_t)0x01)
#define  MPU_ACCESS_NOT_SHAREABLE    ((uint8_t)0x00)
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Access_Cacheable CORTEX MPU Instruction Access Cacheable
  * @{
  */
#define  MPU_ACCESS_CACHEABLE         ((uint8_t)0x01)
#define  MPU_ACCESS_NOT_CACHEABLE     ((uint8_t)0x00)
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Access_Bufferable CORTEX MPU Instruction Access Bufferable
  * @{
  */
#define  MPU_ACCESS_BUFFERABLE         ((uint8_t)0x01)
#define  MPU_ACCESS_NOT_BUFFERABLE     ((uint8_t)0x00)
/**
  * @}
  */

/** @defgroup CORTEX_MPU_TEX_Levels MPU TEX Levels
  * @{
  */
#define  MPU_TEX_LEVEL0    ((uint8_t)0x00)
#define  MPU_TEX_LEVEL1    ((uint8_t)0x01)
#define  MPU_TEX_LEVEL2    ((uint8_t)0x02)
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Region_Size CORTEX MPU Region Size
  * @{
  */
#define   MPU_REGION_SIZE_32B      ((uint8_t)0x04)
#define   MPU_REGION_SIZE_64B      ((uint8_t)0x05)
#define   MPU_REGION_SIZE_128B     ((uint8_t)0x06)
#define   MPU_REGION_SIZE_256B     ((uint8_t)0x07)
#define   MPU_REGION_SIZE_512B     ((uint8_t)0x08)
#define   MPU_REGION_SIZE_1KB      ((uint8_t)0x09)
#define   MPU_REGION_SIZE_2KB      ((uint8_t)0x0A)
#define   MPU_REGION_SIZE_4KB      ((uint8_t)0x0B)
#define   MPU_REGION_SIZE_8KB      ((uint8_t)0x0C)
#define   MPU_REGION_SIZE_16KB     ((uint8_t)0x0D)
#define   MPU_REGION_SIZE_32KB     ((uint8_t)0x0E)
#define   MPU_REGION_SIZE_64KB     ((uint8_t)0x0F)
#define   MPU_REGION_SIZE_128KB    ((uint8_t)0x10)
#define   MPU_REGION_SIZE_256KB    ((uint8_t)0x11)
#define   MPU_REGION_SIZE_512KB    ((uint8_t)0x12)
#define   MPU_REGION_SIZE_1MB      ((uint8_t)0x13)
#define   MPU_REGION_SIZE_2MB      ((uint8_t)0x14)
#define   MPU_REGION_SIZE_4MB      ((uint8_t)0x15)
#define   MPU_REGION_SIZE_8MB      ((uint8_t)0x16)
#define   MPU_REGION_SIZE_16MB     ((uint8_t)0x17)
#define   MPU_REGION_SIZE_32MB     ((uint8_t)0x18)
#define   MPU_REGION_SIZE_64MB     ((uint8_t)0x19)
#define   MPU_REGION_SIZE_128MB    ((uint8_t)0x1A)
#define   MPU_REGION_SIZE_256MB    ((uint8_t)0x1B)
#define   MPU_REGION_SIZE_512MB    ((uint8_t)0x1C)
#define   MPU_REGION_SIZE_1GB      ((uint8_t)0x1D)
#define   MPU_REGION_SIZE_2GB      ((uint8_t)0x1E)
#define   MPU_REGION_SIZE_4GB      ((uint8_t)0x1F)
/**
  * @}
  */
   
/** @defgroup CORTEX_MPU_Region_Permission_Attributes CORTEX MPU Region Permission Attributes 
  * @{
  */
#define  MPU_REGION_NO_ACCESS      ((uint8_t)0x00)
#define  MPU_REGION_PRIV_RW        ((uint8_t)0x01)
#define  MPU_REGION_PRIV_RW_URO    ((uint8_t)0x02)
#define  MPU_REGION_FULL_ACCESS    ((uint8_t)0x03)
#define  MPU_REGION_PRIV_RO        ((uint8_t)0x05)
#define  MPU_REGION_PRIV_RO_URO    ((uint8_t)0x06)
/**
  * @}
  */

/** @defgroup CORTEX_MPU_Region_Number CORTEX MPU Region Number
  * @{
  */
#define  MPU_REGION_NUMBER0    ((uint8_t)0x00)
#define  MPU_REGION_NUMBER1    ((uint8_t)0x01)
#define  MPU_REGION_NUMBER2    ((uint8_t)0x02)
#define  MPU_REGION_NUMBER3    ((uint8_t)0x03)
#define  MPU_REGION_NUMBER4    ((uint8_t)0x04)
#define  MPU_REGION_NUMBER5    ((uint8_t)0x05)
#define  MPU_REGION_NUMBER6    ((uint8_t)0x06)
#define  MPU_REGION_NUMBER7    ((uint8_t)0x07)
/**
  * @}
  */
#endif /* __MPU_PRESENT */

/**
  * @}
  */


/* Exported Macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup CORTEX_Exported_Functions
  * @{
  */
  
/** @addtogroup CORTEX_Exported_Functions_Group1
  * @{
  */
/* Initialization and de-initialization functions *****************************/
void HAL_NVIC_SetPriorityGrouping(uint32_t PriorityGroup);
void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority);
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn);
void HAL_NVIC_SystemReset(void);
uint32_t HAL_SYSTICK_Config(uint32_t TicksNumb);
/**
  * @}
  */

/** @addtogroup CORTEX_Exported_Functions_Group2
  * @{
  */
/* Peripheral Control functions ***********************************************/
uint32_t HAL_NVIC_GetPriorityGrouping(void);
void HAL_NVIC_GetPriority(IRQn_Type IRQn, uint32_t PriorityGroup, uint32_t* pPreemptPriority, uint32_t* pSubPriority);
uint32_t HAL_NVIC_GetPendingIRQ(IRQn_Type IRQn);
void HAL_NVIC_SetPendingIRQ(IRQn_Type IRQn);
void HAL_NVIC_ClearPendingIRQ(IRQn_Type IRQn);
uint32_t HAL_NVIC_GetActive(IRQn_Type IRQn);
void HAL_SYSTICK_CLKSourceConfig(uint32_t CLKSource);
void HAL_SYSTICK_IRQHandler(void);
void HAL_SYSTICK_Callback(void);

#if (__MPU_PRESENT == 1U)
void HAL_MPU_Enable(uint32_t MPU_Control);
void HAL_MPU_Disable(void);
void HAL_MPU_ConfigRegion(MPU_Region_InitTypeDef *MPU_Init);
#endif /* __MPU_PRESENT */
/**
  * @}
  */

/**
  * @}
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @defgroup CORTEX_Private_Macros CORTEX Private Macros
  * @{
  */
#define IS_NVIC_PRIORITY_GROUP(GROUP) (((GROUP) == NVIC_PRIORITYGROUP_0) || \
                                       ((GROUP) == NVIC_PRIORITYGROUP_1) || \
                                       ((GROUP) == NVIC_PRIORITYGROUP_2) || \
                                       ((GROUP) == NVIC_PRIORITYGROUP_3) || \
                                       ((GROUP) == NVIC_PRIORITYGROUP_4))

#define IS_NVIC_PREEMPTION_PRIORITY(PRIORITY)  ((PRIORITY) < 0x10U)

#define IS_NVIC_SUB_PRIORITY(PRIORITY)         ((PRIORITY) < 0x10U)

#define IS_NVIC_DEVICE_IRQ(IRQ)                ((IRQ) >= (IRQn_Type)0x00U)

#define IS_SYSTICK_CLK_SOURCE(SOURCE) (((SOURCE) == SYSTICK_CLKSOURCE_HCLK) || \
                                       ((SOURCE) == SYSTICK_CLKSOURCE_HCLK_DIV8))

#if (__MPU_PRESENT == 1U)
#define IS_MPU_REGION_ENABLE(STATE) (((STATE) == MPU_REGION_ENABLE) || \
                                     ((STATE) == MPU_REGION_DISABLE))

#define IS_MPU_INSTRUCTION_ACCESS(STATE) (((STATE) == MPU_INSTRUCTION_ACCESS_ENABLE) || \
                                          ((STATE) == MPU_INSTRUCTION_ACCESS_DISABLE))

#define IS_MPU_ACCESS_SHAREABLE(STATE)   (((STATE) == MPU_ACCESS_SHAREABLE) || \
                                          ((STATE) == MPU_ACCESS_NOT_SHAREABLE))

#define IS_MPU_ACCESS_CACHEABLE(STATE)   (((STATE) == MPU_ACCESS_CACHEABLE) || \
                                          ((STATE) == MPU_ACCESS_NOT_CACHEABLE))

#define IS_MPU_ACCESS_BUFFERABLE(STATE)   (((STATE) == MPU_ACCESS_BUFFERABLE) || \
                                          ((STATE) == MPU_ACCESS_NOT_BUFFERABLE))

#define IS_MPU_TEX_LEVEL(TYPE) (((TYPE) == MPU_TEX_LEVEL0)  || \
                                ((TYPE) == MPU_TEX_LEVEL1)  || \
                                ((TYPE) == MPU_TEX_LEVEL2))

#define IS_MPU_REGION_PERMISSION_ATTRIBUTE(TYPE) (((TYPE) == MPU_REGION_NO_ACCESS)   || \
                                                  ((TYPE) == MPU_REGION_PRIV_RW)     || \
                                                  ((TYPE) == MPU_REGION_PRIV_RW_URO) || \
                                                  ((TYPE) == MPU_REGION_FULL_ACCESS) || \
                                                  ((TYPE) == MPU_REGION_PRIV_RO)     || \
                                                  ((TYPE) == MPU_REGION_PRIV_RO_URO))

#define IS_MPU_REGION_NUMBER(NUMBER)    (((NUMBER) == MPU_REGION_NUMBER0) || \
                                         ((NUMBER) == MPU_REGION_NUMBER1) || \
                                         ((NUMBER) == MPU_REGION_NUMBER2) || \
                                         ((NUMBER) == MPU_REGION_NUMBER3) || \
                                         ((NUMBER) == MPU_REGION_NUMBER4) || \
                                         ((NUMBER) == MPU_REGION_NUMBER5) || \
                                         ((NUMBER) == MPU_REGION_NUMBER6) || \
                                         ((NUMBER) == MPU_REGION_NUMBER7))

#define IS_MPU_REGION_SIZE(SIZE)    (((SIZE) == MPU_REGION_SIZE_32B)   || \
                                     ((SIZE) == MPU_REGION_SIZE_64B)   || \
                                     ((SIZE) == MPU_REGION_SIZE_128B)  || \
                                     ((SIZE) == MPU_REGION_SIZE_256B)  || \
                                     ((SIZE) == MPU_REGION_SIZE_512B)  || \
                                     ((SIZE) == MPU_REGION_SIZE_1KB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_2KB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_4KB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_8KB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_16KB)  || \
                                     ((SIZE) == MPU_REGION_SIZE_32KB)  || \
                                     ((SIZE) == MPU_REGION_SIZE_64KB)  || \
                                     ((SIZE) == MPU_REGION_SIZE_128KB) || \
                                     ((SIZE) == MPU_REGION_SIZE_256KB) || \
                                     ((SIZE) == MPU_REGION_SIZE_512KB) || \
                                     ((SIZE) == MPU_REGION_SIZE_1MB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_2MB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_4MB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_8MB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_16MB)  || \
                                     ((SIZE) == MPU_REGION_SIZE_32MB)  || \
                                     ((SIZE) == MPU_REGION_SIZE_64MB)  || \
                                     ((SIZE) == MPU_REGION_SIZE_128MB) || \
                                     ((SIZE) == MPU_REGION_SIZE_256MB) || \
                                     ((SIZE) == MPU_REGION_SIZE_512MB) || \
                                     ((SIZE) == MPU_REGION_SIZE_1GB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_2GB)   || \
                                     ((SIZE) == MPU_REGION_SIZE_4GB))

#define IS_MPU_SUB_REGION_DISABLE(SUBREGION)  ((SUBREGION) < (uint16_t)0x00FF)
#endif /* __MPU_PRESENT */

/**                                                                          
  * @}                                                                  
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @}
  */ 

/**
  * @}
  */
  
#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_CORTEX_H */
 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/stm32f1xx_hal_gpio_ex.h`

```c

/* excerpt lines 1-60 */
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_gpio_ex.h
  * @author  MCD Application Team
  * @brief   Header file of GPIO HAL Extension module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32F1xx_HAL_GPIO_EX_H
#define STM32F1xx_HAL_GPIO_EX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal_def.h"

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @defgroup GPIOEx GPIOEx
  * @{
  */
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** @defgroup GPIOEx_Exported_Constants GPIOEx Exported Constants
  * @{
  */

/** @defgroup GPIOEx_EVENTOUT EVENTOUT Cortex Configuration
  * @brief This section propose definition to use the Cortex EVENTOUT signal.
  * @{
  */

/** @defgroup GPIOEx_EVENTOUT_PIN EVENTOUT Pin
  * @{
  */

#define AFIO_EVENTOUT_PIN_0  AFIO_EVCR_PIN_PX0 /*!< EVENTOUT on pin 0 */
#define AFIO_EVENTOUT_PIN_1  AFIO_EVCR_PIN_PX1 /*!< EVENTOUT on pin 1 */
#define AFIO_EVENTOUT_PIN_2  AFIO_EVCR_PIN_PX2 /*!< EVENTOUT on pin 2 */
#define AFIO_EVENTOUT_PIN_3  AFIO_EVCR_PIN_PX3 /*!< EVENTOUT on pin 3 */
#define AFIO_EVENTOUT_PIN_4  AFIO_EVCR_PIN_PX4 /*!< EVENTOUT on pin 4 */
#define AFIO_EVENTOUT_PIN_5  AFIO_EVCR_PIN_PX5 /*!< EVENTOUT on pin 5 */
#define AFIO_EVENTOUT_PIN_6  AFIO_EVCR_PIN_PX6 /*!< EVENTOUT on pin 6 */

/* excerpt lines 104-162 */
                                         ((__PORT__) == AFIO_EVENTOUT_PORT_D) || \
                                         ((__PORT__) == AFIO_EVENTOUT_PORT_E))
/**
  * @}
  */

/**
  * @}
  */

/** @defgroup GPIOEx_AFIO_AF_REMAPPING Alternate Function Remapping
  * @brief This section propose definition to remap the alternate function to some other port/pins.
  * @{
  */

/**
  * @brief Enable the remapping of SPI1 alternate function NSS, SCK, MISO and MOSI.
  * @note  ENABLE: Remap     (NSS/PA15, SCK/PB3, MISO/PB4, MOSI/PB5)
  * @retval None
  */
#define __HAL_AFIO_REMAP_SPI1_ENABLE()  AFIO_REMAP_ENABLE(AFIO_MAPR_SPI1_REMAP)

/**
  * @brief Disable the remapping of SPI1 alternate function NSS, SCK, MISO and MOSI.
  * @note  DISABLE: No remap (NSS/PA4,  SCK/PA5, MISO/PA6, MOSI/PA7)
  * @retval None
  */
#define __HAL_AFIO_REMAP_SPI1_DISABLE()  AFIO_REMAP_DISABLE(AFIO_MAPR_SPI1_REMAP)

/**
  * @brief Enable the remapping of I2C1 alternate function SCL and SDA.
  * @note  ENABLE: Remap     (SCL/PB8, SDA/PB9)
  * @retval None
  */
#define __HAL_AFIO_REMAP_I2C1_ENABLE()  AFIO_REMAP_ENABLE(AFIO_MAPR_I2C1_REMAP)

/**
  * @brief Disable the remapping of I2C1 alternate function SCL and SDA.
  * @note  DISABLE: No remap (SCL/PB6, SDA/PB7)
  * @retval None
  */
#define __HAL_AFIO_REMAP_I2C1_DISABLE() AFIO_REMAP_DISABLE(AFIO_MAPR_I2C1_REMAP)

/**
  * @brief Enable the remapping of USART1 alternate function TX and RX.
  * @note  ENABLE: Remap     (TX/PB6, RX/PB7)
  * @retval None
  */
#define __HAL_AFIO_REMAP_USART1_ENABLE()  AFIO_REMAP_ENABLE(AFIO_MAPR_USART1_REMAP)

/**
  * @brief Disable the remapping of USART1 alternate function TX and RX.
  * @note  DISABLE: No remap (TX/PA9, RX/PA10)
  * @retval None
  */
#define __HAL_AFIO_REMAP_USART1_DISABLE() AFIO_REMAP_DISABLE(AFIO_MAPR_USART1_REMAP)

/**
  * @brief Enable the remapping of USART2 alternate function CTS, RTS, CK, TX and RX.

/* excerpt lines 478-541 */

/**
  * @brief Enable the Serial wire JTAG configuration
  * @note  NOJTAG: JTAG-DP Disabled and SW-DP Enabled
  * @retval None
  */

#define __HAL_AFIO_REMAP_SWJ_NOJTAG()  AFIO_DBGAFR_CONFIG(AFIO_MAPR_SWJ_CFG_JTAGDISABLE)

/**
  * @brief Disable the Serial wire JTAG configuration
  * @note  DISABLE: JTAG-DP Disabled and SW-DP Disabled
  * @retval None
  */
#define __HAL_AFIO_REMAP_SWJ_DISABLE()  AFIO_DBGAFR_CONFIG(AFIO_MAPR_SWJ_CFG_DISABLE)

#if defined(AFIO_MAPR_SPI3_REMAP)

/**
  * @brief Enable the remapping of SPI3 alternate functions SPI3_NSS/I2S3_WS, SPI3_SCK/I2S3_CK, SPI3_MISO, SPI3_MOSI/I2S3_SD.
  * @note  ENABLE: Remap     (SPI3_NSS-I2S3_WS/PA4,  SPI3_SCK-I2S3_CK/PC10, SPI3_MISO/PC11, SPI3_MOSI-I2S3_SD/PC12)
  * @note  This bit is available only in connectivity line devices and is reserved otherwise.
  * @retval None
  */
#define __HAL_AFIO_REMAP_SPI3_ENABLE()  AFIO_REMAP_ENABLE(AFIO_MAPR_SPI3_REMAP)

/**
  * @brief Disable the remapping of SPI3 alternate functions SPI3_NSS/I2S3_WS, SPI3_SCK/I2S3_CK, SPI3_MISO, SPI3_MOSI/I2S3_SD.
  * @note  DISABLE: No remap (SPI3_NSS-I2S3_WS/PA15, SPI3_SCK-I2S3_CK/PB3,  SPI3_MISO/PB4,  SPI3_MOSI-I2S3_SD/PB5).
  * @note  This bit is available only in connectivity line devices and is reserved otherwise.
  * @retval None
  */
#define __HAL_AFIO_REMAP_SPI3_DISABLE() AFIO_REMAP_DISABLE(AFIO_MAPR_SPI3_REMAP)
#endif

#if defined(AFIO_MAPR_TIM2ITR1_IREMAP)

/**
  * @brief Control of TIM2_ITR1 internal mapping.
  * @note  TO_USB: Connect USB OTG SOF (Start of Frame) output to TIM2_ITR1 for calibration purposes.
  * @note  This bit is available only in connectivity line devices and is reserved otherwise.
  * @retval None
  */
#define __HAL_AFIO_TIM2ITR1_TO_USB() AFIO_REMAP_ENABLE(AFIO_MAPR_TIM2ITR1_IREMAP)

/**
  * @brief Control of TIM2_ITR1 internal mapping.
  * @note  TO_ETH: Connect TIM2_ITR1 internally to the Ethernet PTP output for calibration purposes.
  * @note  This bit is available only in connectivity line devices and is reserved otherwise.
  * @retval None
  */
#define __HAL_AFIO_TIM2ITR1_TO_ETH() AFIO_REMAP_DISABLE(AFIO_MAPR_TIM2ITR1_IREMAP)
#endif

#if defined(AFIO_MAPR_PTP_PPS_REMAP)

/**
  * @brief Enable the remapping of ADC2_ETRGREG (ADC 2 External trigger regular conversion).
  * @note  ENABLE: PTP_PPS is output on PB5 pin.
  * @note  This bit is available only in connectivity line devices and is reserved otherwise.
  * @retval None
  */
#define __HAL_AFIO_ETH_PTP_PPS_ENABLE()  AFIO_REMAP_ENABLE(AFIO_MAPR_PTP_PPS_REMAP)


/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/src/stm32f1xx_hal.c`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal.c
  * @author  MCD Application Team
  * @brief   HAL module driver.
  *          This is the common part of the HAL initialization
  *
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
    [..]
    The common HAL driver contains a set of generic and common APIs that can be
    used by the PPP peripheral drivers and the user to start using the HAL.
    [..]
    The HAL contains two APIs' categories:
         (+) Common HAL APIs
         (+) Services HAL APIs

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stm32f1xx_hal.h>

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @defgroup HAL HAL
  * @brief HAL module driver.
  * @{
  */

#ifdef HAL_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/** @defgroup HAL_Private_Constants HAL Private Constants
  * @{
  */
/**
 * @brief STM32F1xx HAL Driver version number V1.1.4
   */
#define __STM32F1xx_HAL_VERSION_MAIN   (0x01U) /*!< [31:24] main version */
#define __STM32F1xx_HAL_VERSION_SUB1   (0x01U) /*!< [23:16] sub1 version */
#define __STM32F1xx_HAL_VERSION_SUB2   (0x04U) /*!< [15:8]  sub2 version */
#define __STM32F1xx_HAL_VERSION_RC     (0x00U) /*!< [7:0]  release candidate */
#define __STM32F1xx_HAL_VERSION         ((__STM32F1xx_HAL_VERSION_MAIN << 24)\
                                        |(__STM32F1xx_HAL_VERSION_SUB1 << 16)\
                                        |(__STM32F1xx_HAL_VERSION_SUB2 << 8 )\
                                        |(__STM32F1xx_HAL_VERSION_RC))

#define IDCODE_DEVID_MASK    0x00000FFFU

/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/** @defgroup HAL_Private_Variables HAL Private Variables
  * @{
  */
__IO uint32_t uwTick;
uint32_t uwTickPrio   = (1UL << __NVIC_PRIO_BITS); /* Invalid PRIO */
HAL_TickFreqTypeDef uwTickFreq = HAL_TICK_FREQ_DEFAULT;  /* 1KHz */
/**
  * @}
  */
/* Private function prototypes -----------------------------------------------*/
/* Exported functions ---------------------------------------------------------*/

/** @defgroup HAL_Exported_Functions HAL Exported Functions
  * @{
  */

/** @defgroup HAL_Exported_Functions_Group1 Initialization and de-initialization Functions
 *  @brief    Initialization and de-initialization functions
 *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
   [..]  This section provides functions allowing to:
      (+) Initializes the Flash interface, the NVIC allocation and initial clock
          configuration. It initializes the systick also when timeout is needed
          and the backup domain when enabled.
      (+) de-Initializes common part of the HAL.
      (+) Configure The time base source to have 1ms time base with a dedicated
          Tick interrupt priority.
        (++) SysTick timer is used by default as source of time base, but user
             can eventually implement his proper time base source (a general purpose
             timer for example or other time source), keeping in mind that Time base
             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
             handled in milliseconds basis.
        (++) Time base configuration function (HAL_InitTick ()) is called automatically
             at the beginning of the program after reset by HAL_Init() or at any time
             when clock is configured, by HAL_RCC_ClockConfig().
        (++) Source of time base is configured  to generate interrupts at regular
             time intervals. Care must be taken if HAL_Delay() is called from a
             peripheral ISR process, the Tick interrupt line must have higher priority
            (numerically lower) than the peripheral interrupt. Otherwise the caller
            ISR process will be blocked.
       (++) functions affecting time base configurations are declared as __weak
             to make  override possible  in case of other  implementations in user file.
@endverbatim
  * @{
  */

/**
  * @brief  This function is used to initialize the HAL Library; it must be the first
  *         instruction to be executed in the main program (before to call any other
  *         HAL function), it performs the following:
  *           Configure the Flash prefetch.
  *           Configures the SysTick to generate an interrupt each 1 millisecond,
  *           which is clocked by the HSI (at this stage, the clock is not yet
  *           configured and thus the system is running from the internal HSI at 16 MHz).
  *           Set NVIC Group Priority to 4.
  *           Calls the HAL_MspInit() callback function defined in user file
  *           "stm32f1xx_hal_msp.c" to do the global low level hardware initialization
  *
  * @note   SysTick is used as time base for the HAL_Delay() function, the application
  *         need to ensure that the SysTick time base is always set to 1 millisecond
  *         to have correct HAL operation.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_Init(void)
{
  /* Configure Flash prefetch */
#if (PREFETCH_ENABLE != 0)
#if defined(STM32F101x6) || defined(STM32F101xB) || defined(STM32F101xE) || defined(STM32F101xG) || \
    defined(STM32F102x6) || defined(STM32F102xB) || \
    defined(STM32F103x6) || defined(STM32F103xB) || defined(STM32F103xE) || defined(STM32F103xG) || \
    defined(STM32F105xC) || defined(STM32F107xC)

  /* Prefetch buffer is not available on value line devices */
  __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif
#endif /* PREFETCH_ENABLE */

  /* Set Interrupt Group Priority */
  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* Use systick as time base source and configure 1ms tick (default clock after Reset is HSI) */
  HAL_InitTick(TICK_INT_PRIORITY);

  /* Init the low level hardware */
  HAL_MspInit();

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief This function de-Initializes common part of the HAL and stops the systick.
  *        of time base.
  * @note This function is optional.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DeInit(void)
{
  /* Reset of all peripherals */
  __HAL_RCC_APB1_FORCE_RESET();
  __HAL_RCC_APB1_RELEASE_RESET();

  __HAL_RCC_APB2_FORCE_RESET();
  __HAL_RCC_APB2_RELEASE_RESET();

#if defined(STM32F105xC) || defined(STM32F107xC)
  __HAL_RCC_AHB_FORCE_RESET();
  __HAL_RCC_AHB_RELEASE_RESET();
#endif

  /* De-Init the low level hardware */
  HAL_MspDeInit();

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Initialize the MSP.
  * @retval None
  */
__weak void HAL_MspInit(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_MspInit could be implemented in the user file
   */
}

/**
  * @brief  DeInitializes the MSP.
  * @retval None
  */
__weak void HAL_MspDeInit(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_MspDeInit could be implemented in the user file
   */
}

/**
  * @brief This function configures the source of the time base.
  *        The time source is configured  to have 1ms time base with a dedicated
  *        Tick interrupt priority.
  * @note This function is called  automatically at the beginning of program after
  *       reset by HAL_Init() or at any time when clock is reconfigured  by HAL_RCC_ClockConfig().
  * @note In the default implementation, SysTick timer is the source of time base.
  *       It is used to generate interrupts at regular time intervals.
  *       Care must be taken if HAL_Delay() is called from a peripheral ISR process,
  *       The SysTick interrupt must have higher priority (numerically lower)
  *       than the peripheral interrupt. Otherwise the caller ISR process will be blocked.
  *       The function is declared as __weak  to be overwritten  in case of other
  *       implementation  in user file.
  * @param TickPriority Tick interrupt priority.
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  /* Configure the SysTick to have interrupt in 1ms time basis*/
  if (HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq)) > 0U)
  {
    return HAL_ERROR;
  }

  /* Configure the SysTick IRQ priority */
  if (TickPriority < (1UL << __NVIC_PRIO_BITS))
  {
    HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority, 0U);
    uwTickPrio = TickPriority;
  }
  else
  {
    return HAL_ERROR;
  }

  /* Return function status */
  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup HAL_Exported_Functions_Group2 HAL Control functions
  *  @brief    HAL Control functions
  *
@verbatim
 ===============================================================================
                      ##### HAL Control functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Provide a tick value in millisecond
      (+) Provide a blocking delay in millisecond
      (+) Suspend the time base source interrupt
      (+) Resume the time base source interrupt
      (+) Get the HAL API driver version
      (+) Get the device identifier
      (+) Get the device revision identifier
      (+) Enable/Disable Debug module during SLEEP mode
      (+) Enable/Disable Debug module during STOP mode
      (+) Enable/Disable Debug module during STANDBY mode

@endverbatim
  * @{
  */

/**
  * @brief This function is called to increment  a global variable "uwTick"
  *        used as application time base.
  * @note In the default implementation, this variable is incremented each 1ms
  *       in SysTick ISR.
  * @note This function is declared as __weak to be overwritten in case of other
  *      implementations in user file.
  * @retval None
  */
__weak void HAL_IncTick(void)
{
  uwTick += uwTickFreq;
}

/**
  * @brief Provides a tick value in millisecond.
  * @note  This function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @retval tick value
  */
__weak uint32_t HAL_GetTick(void)
{
  return uwTick;
}

/**
  * @brief This function returns a tick priority.
  * @retval tick priority
  */
uint32_t HAL_GetTickPrio(void)
{
  return uwTickPrio;
}

/**
  * @brief Set new tick Freq.
  * @retval status
  */
HAL_StatusTypeDef HAL_SetTickFreq(HAL_TickFreqTypeDef Freq)
{
  HAL_StatusTypeDef status  = HAL_OK;
  assert_param(IS_TICKFREQ(Freq));

  if (uwTickFreq != Freq)
  {
    /* Apply the new tick Freq  */
    status = HAL_InitTick(uwTickPrio);
    if (status == HAL_OK)
    {
      uwTickFreq = Freq;
    }
  }

  return status;
}

/**
  * @brief Return tick frequency.
  * @retval tick period in Hz
  */
HAL_TickFreqTypeDef HAL_GetTickFreq(void)
{
  return uwTickFreq;
}

/**
  * @brief This function provides minimum delay (in milliseconds) based
  *        on variable incremented.
  * @note In the default implementation , SysTick timer is the source of time base.
  *       It is used to generate interrupts at regular time intervals where uwTick
  *       is incremented.
  * @note This function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @param Delay specifies the delay time length, in milliseconds.
  * @retval None
  */
__weak void HAL_Delay(uint32_t Delay)
{
  uint32_t tickstart = HAL_GetTick();
  uint32_t wait = Delay;

  /* Add a freq to guarantee minimum wait */
  if (wait < HAL_MAX_DELAY)
  {
    wait += (uint32_t)(uwTickFreq);
  }

  while ((HAL_GetTick() - tickstart) < wait)
  {
  }
}

/**
  * @brief Suspend Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once HAL_SuspendTick()
  *       is called, the SysTick interrupt will be disabled and so Tick increment
  *       is suspended.
  * @note This function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @retval None
  */
__weak void HAL_SuspendTick(void)
{
  /* Disable SysTick Interrupt */
  CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
}

/**
  * @brief Resume Tick increment.
  * @note In the default implementation , SysTick timer is the source of time base. It is
  *       used to generate interrupts at regular time intervals. Once HAL_ResumeTick()
  *       is called, the SysTick interrupt will be enabled and so Tick increment
  *       is resumed.
  * @note This function is declared as __weak to be overwritten in case of other
  *       implementations in user file.
  * @retval None
  */
__weak void HAL_ResumeTick(void)
{
  /* Enable SysTick Interrupt */
  SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
}

/**
  * @brief  Returns the HAL revision
  * @retval version 0xXYZR (8bits for each decimal, R for RC)
  */
uint32_t HAL_GetHalVersion(void)
{
  return __STM32F1xx_HAL_VERSION;
}

/**
  * @brief Returns the device revision identifier.
  * Note: On devices STM32F10xx8 and STM32F10xxB,
  *                  STM32F101xC/D/E and STM32F103xC/D/E,
  *                  STM32F101xF/G and STM32F103xF/G
  *                  STM32F10xx4 and STM32F10xx6
  *       Debug registers DBGMCU_IDCODE and DBGMCU_CR are accessible only in
  *       debug mode (not accessible by the user software in normal mode).
  *       Refer to errata sheet of these devices for more details.
  * @retval Device revision identifier
  */
uint32_t HAL_GetREVID(void)
{
  return ((DBGMCU->IDCODE) >> DBGMCU_IDCODE_REV_ID_Pos);
}

/**
  * @brief  Returns the device identifier.
  * Note: On devices STM32F10xx8 and STM32F10xxB,
  *                  STM32F101xC/D/E and STM32F103xC/D/E,
  *                  STM32F101xF/G and STM32F103xF/G
  *                  STM32F10xx4 and STM32F10xx6
  *       Debug registers DBGMCU_IDCODE and DBGMCU_CR are accessible only in
  *       debug mode (not accessible by the user software in normal mode).
  *       Refer to errata sheet of these devices for more details.
  * @retval Device identifier
  */
uint32_t HAL_GetDEVID(void)
{
  return ((DBGMCU->IDCODE) & IDCODE_DEVID_MASK);
}

/**
  * @brief  Returns first word of the unique device identifier (UID based on 96 bits)
  * @retval Device identifier
  */
uint32_t HAL_GetUIDw0(void)
{
   return(READ_REG(*((uint32_t *)UID_BASE)));
}

/**
  * @brief  Returns second word of the unique device identifier (UID based on 96 bits)
  * @retval Device identifier
  */
uint32_t HAL_GetUIDw1(void)
{
   return(READ_REG(*((uint32_t *)(UID_BASE + 4U))));
}

/**
  * @brief  Returns third word of the unique device identifier (UID based on 96 bits)
  * @retval Device identifier
  */
uint32_t HAL_GetUIDw2(void)
{
   return(READ_REG(*((uint32_t *)(UID_BASE + 8U))));
}

/**
  * @brief  Enable the Debug Module during SLEEP mode
  * @retval None
  */
void HAL_DBGMCU_EnableDBGSleepMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

/**
  * @brief  Disable the Debug Module during SLEEP mode
  * Note: On devices STM32F10xx8 and STM32F10xxB,
  *                  STM32F101xC/D/E and STM32F103xC/D/E,
  *                  STM32F101xF/G and STM32F103xF/G
  *                  STM32F10xx4 and STM32F10xx6
  *       Debug registers DBGMCU_IDCODE and DBGMCU_CR are accessible only in
  *       debug mode (not accessible by the user software in normal mode).
  *       Refer to errata sheet of these devices for more details.
  * @retval None
  */
void HAL_DBGMCU_DisableDBGSleepMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

/**
  * @brief  Enable the Debug Module during STOP mode
  * Note: On devices STM32F10xx8 and STM32F10xxB,
  *                  STM32F101xC/D/E and STM32F103xC/D/E,
  *                  STM32F101xF/G and STM32F103xF/G
  *                  STM32F10xx4 and STM32F10xx6
  *       Debug registers DBGMCU_IDCODE and DBGMCU_CR are accessible only in
  *       debug mode (not accessible by the user software in normal mode).
  *       Refer to errata sheet of these devices for more details.
  * Note: On all STM32F1 devices:
  *       If the system tick timer interrupt is enabled during the Stop mode
  *       debug (DBG_STOP bit set in the DBGMCU_CR register ), it will wakeup
  *       the system from Stop mode.
  *       Workaround: To debug the Stop mode, disable the system tick timer
  *       interrupt.
  *       Refer to errata sheet of these devices for more details.
  * Note: On all STM32F1 devices:
  *       If the system tick timer interrupt is enabled during the Stop mode
  *       debug (DBG_STOP bit set in the DBGMCU_CR register ), it will wakeup
  *       the system from Stop mode.
  *       Workaround: To debug the Stop mode, disable the system tick timer
  *       interrupt.
  *       Refer to errata sheet of these devices for more details.
  * @retval None
  */
void HAL_DBGMCU_EnableDBGStopMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

/**
  * @brief  Disable the Debug Module during STOP mode
  * Note: On devices STM32F10xx8 and STM32F10xxB,
  *                  STM32F101xC/D/E and STM32F103xC/D/E,
  *                  STM32F101xF/G and STM32F103xF/G
  *                  STM32F10xx4 and STM32F10xx6
  *       Debug registers DBGMCU_IDCODE and DBGMCU_CR are accessible only in
  *       debug mode (not accessible by the user software in normal mode).
  *       Refer to errata sheet of these devices for more details.
  * @retval None
  */
void HAL_DBGMCU_DisableDBGStopMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

/**
  * @brief  Enable the Debug Module during STANDBY mode
  * Note: On devices STM32F10xx8 and STM32F10xxB,
  *                  STM32F101xC/D/E and STM32F103xC/D/E,
  *                  STM32F101xF/G and STM32F103xF/G
  *                  STM32F10xx4 and STM32F10xx6
  *       Debug registers DBGMCU_IDCODE and DBGMCU_CR are accessible only in
  *       debug mode (not accessible by the user software in normal mode).
  *       Refer to errata sheet of these devices for more details.
  * @retval None
  */
void HAL_DBGMCU_EnableDBGStandbyMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

/**
  * @brief  Disable the Debug Module during STANDBY mode
  * Note: On devices STM32F10xx8 and STM32F10xxB,
  *                  STM32F101xC/D/E and STM32F103xC/D/E,
  *                  STM32F101xF/G and STM32F103xF/G
  *                  STM32F10xx4 and STM32F10xx6
  *       Debug registers DBGMCU_IDCODE and DBGMCU_CR are accessible only in
  *       debug mode (not accessible by the user software in normal mode).
  *       Refer to errata sheet of these devices for more details.
  * @retval None
  */
void HAL_DBGMCU_DisableDBGStandbyMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/include/core_cm3.h`

```c

/* excerpt lines 1-60 */
/**************************************************************************//**
 * @file     core_cm3.h
 * @brief    CMSIS Cortex-M3 Core Peripheral Access Layer Header File
 * @version  V5.0.8
 * @date     04. June 2018
 ******************************************************************************/
/*
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if   defined ( __ICCARM__ )
  #pragma system_include         /* treat file as system include file for MISRA check */
#elif defined (__clang__)
  #pragma clang system_header   /* treat file as system include file */
#endif

#ifndef __CORE_CM3_H_GENERIC
#define __CORE_CM3_H_GENERIC

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/**
  \page CMSIS_MISRA_Exceptions  MISRA-C:2004 Compliance Exceptions
  CMSIS violates the following MISRA-C:2004 rules:

   \li Required Rule 8.5, object/function definition in header file.<br>
     Function definitions in header files are used to allow 'inlining'.

   \li Required Rule 18.4, declaration of union type or object of union type: '{...}'.<br>
     Unions are used for effective representation of core registers.

   \li Advisory Rule 19.7, Function-like macro defined.<br>
     Function-like macros are used to allow more efficient code.
 */


/*******************************************************************************
 *                 CMSIS definitions
 ******************************************************************************/
/**
  \ingroup Cortex_M3
  @{

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/include/cmsis_compiler.h`

```c
/**************************************************************************//**
 * @file     cmsis_compiler.h
 * @brief    CMSIS compiler generic header file
 * @version  V5.0.4
 * @date     10. January 2018
 ******************************************************************************/
/*
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CMSIS_COMPILER_H
#define __CMSIS_COMPILER_H

#include <stdint.h>

/*
 * Arm Compiler 4/5
 */
#if   defined ( __CC_ARM )
  #include "cmsis_armcc.h"


/*
 * Arm Compiler 6 (armclang)
 */
#elif defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #include "cmsis_armclang.h"


/*
 * GNU Compiler
 */
#elif defined ( __GNUC__ )
  #include "cmsis_gcc.h"


/*
 * IAR Compiler
 */
#elif defined ( __ICCARM__ )
  #include <cmsis_iccarm.h>


/*
 * TI Arm Compiler
 */
#elif defined ( __TI_ARM__ )
  #include <cmsis_ccs.h>

  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __STATIC_INLINE
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __attribute__((noreturn))
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __attribute__((packed))
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        struct __attribute__((packed))
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         union __attribute__((packed))
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    struct __attribute__((packed)) T_UINT32 { uint32_t v; };
    #define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void*)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)                           __attribute__((aligned(x)))
  #endif
  #ifndef   __RESTRICT
    #warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.
    #define __RESTRICT
  #endif


/*
 * TASKING Compiler
 */
#elif defined ( __TASKING__ )
  /*
   * The CMSIS functions have been implemented as intrinsics in the compiler.
   * Please use "carm -?i" to get an up to date list of all intrinsics,
   * Including the CMSIS ones.
   */

  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __STATIC_INLINE
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __attribute__((noreturn))
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __packed__
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        struct __packed__
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         union __packed__
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    struct __packed__ T_UINT32 { uint32_t v; };
    #define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)              __align(x)
  #endif
  #ifndef   __RESTRICT
    #warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.
    #define __RESTRICT
  #endif


/*
 * COSMIC Compiler
 */
#elif defined ( __CSMC__ )
   #include <cmsis_csm.h>

 #ifndef   __ASM
    #define __ASM                                  _asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __STATIC_INLINE
  #endif
  #ifndef   __NO_RETURN
    // NO RETURN is automatically detected hence no warning here
    #define __NO_RETURN
  #endif
  #ifndef   __USED
    #warning No compiler specific solution for __USED. __USED is ignored.
    #define __USED
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __weak
  #endif
  #ifndef   __PACKED
    #define __PACKED                               @packed
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        @packed struct
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         @packed union
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    @packed struct T_UINT32 { uint32_t v; };
    #define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #warning No compiler specific solution for __ALIGNED. __ALIGNED is ignored.
    #define __ALIGNED(x)
  #endif
  #ifndef   __RESTRICT
    #warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.
    #define __RESTRICT
  #endif


#else
  #error Unknown compiler.
#endif


#endif /* __CMSIS_COMPILER_H */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/include/cmsis_gcc.h`

```c

/* excerpt lines 1-60 */
/**************************************************************************//**
 * @file     cmsis_gcc.h
 * @brief    CMSIS compiler GCC header file
 * @version  V5.0.4
 * @date     09. April 2018
 ******************************************************************************/
/*
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CMSIS_GCC_H
#define __CMSIS_GCC_H

/* ignore some GCC warnings */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* Fallback for __has_builtin */
#ifndef __has_builtin
  #define __has_builtin(x) (0)
#endif

/* CMSIS compiler specific defines */
#ifndef   __ASM
  #define __ASM                                  __asm
#endif
#ifndef   __INLINE
  #define __INLINE                               inline
#endif
#ifndef   __STATIC_INLINE
  #define __STATIC_INLINE                        static inline
#endif
#ifndef   __STATIC_FORCEINLINE                 
  #define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static inline
#endif                                           
#ifndef   __NO_RETURN
  #define __NO_RETURN                            __attribute__((__noreturn__))
#endif
#ifndef   __USED
  #define __USED                                 __attribute__((used))
#endif
#ifndef   __WEAK
  #define __WEAK                                 __attribute__((weak))
#endif

/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/include/cmsis_version.h`

```c
/**************************************************************************//**
 * @file     cmsis_version.h
 * @brief    CMSIS Core(M) Version definitions
 * @version  V5.0.2
 * @date     19. April 2017
 ******************************************************************************/
/*
 * Copyright (c) 2009-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if   defined ( __ICCARM__ )
  #pragma system_include         /* treat file as system include file for MISRA check */
#elif defined (__clang__)
  #pragma clang system_header   /* treat file as system include file */
#endif

#ifndef __CMSIS_VERSION_H
#define __CMSIS_VERSION_H

/*  CMSIS Version definitions */
#define __CM_CMSIS_VERSION_MAIN  ( 5U)                                      /*!< [31:16] CMSIS Core(M) main version */
#define __CM_CMSIS_VERSION_SUB   ( 1U)                                      /*!< [15:0]  CMSIS Core(M) sub version */
#define __CM_CMSIS_VERSION       ((__CM_CMSIS_VERSION_MAIN << 16U) | \
                                   __CM_CMSIS_VERSION_SUB           )       /*!< CMSIS Core(M) version number */
#endif
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/include/stm32f1xx.h`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx.h
  * @author  MCD Application Team
  * @brief   CMSIS STM32F1xx Device Peripheral Access Layer Header File. 
  *
  *          The file is the unique include file that the application programmer
  *          is using in the C source code, usually in main.c. This file contains:
  *            - Configuration section that allows to select:
  *              - The STM32F1xx device used in the target application
  *              - To use or not the peripheral�s drivers in application code(i.e. 
  *                code will be based on direct access to peripheral�s registers 
  *                rather than drivers API), this option is controlled by 
  *                "#define USE_HAL_DRIVER"
  *  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32f1xx
  * @{
  */
    
#ifndef __STM32F1XX_H
#define __STM32F1XX_H

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */
  
/** @addtogroup Library_configuration_section
  * @{
  */

/**
  * @brief STM32 Family
  */
#if !defined (STM32F1)
#define STM32F1
#endif /* STM32F1 */

#if !defined (STM32F103xB)
#define STM32F103xB
#define USE_HAL_DRIVER
#endif

/* Uncomment the line below according to the target STM32L device used in your 
   application 
  */

#if !defined (STM32F100xB) && !defined (STM32F100xE) && !defined (STM32F101x6) && \
    !defined (STM32F101xB) && !defined (STM32F101xE) && !defined (STM32F101xG) && !defined (STM32F102x6) && !defined (STM32F102xB) && !defined (STM32F103x6) && \
    !defined (STM32F103xB) && !defined (STM32F103xE) && !defined (STM32F103xG) && !defined (STM32F105xC) && !defined (STM32F107xC)
  /* #define STM32F100xB  */   /*!< STM32F100C4, STM32F100R4, STM32F100C6, STM32F100R6, STM32F100C8, STM32F100R8, STM32F100V8, STM32F100CB, STM32F100RB and STM32F100VB */
  /* #define STM32F100xE */    /*!< STM32F100RC, STM32F100VC, STM32F100ZC, STM32F100RD, STM32F100VD, STM32F100ZD, STM32F100RE, STM32F100VE and STM32F100ZE */
  /* #define STM32F101x6  */   /*!< STM32F101C4, STM32F101R4, STM32F101T4, STM32F101C6, STM32F101R6 and STM32F101T6 Devices */
  /* #define STM32F101xB  */   /*!< STM32F101C8, STM32F101R8, STM32F101T8, STM32F101V8, STM32F101CB, STM32F101RB, STM32F101TB and STM32F101VB */
  /* #define STM32F101xE */    /*!< STM32F101RC, STM32F101VC, STM32F101ZC, STM32F101RD, STM32F101VD, STM32F101ZD, STM32F101RE, STM32F101VE and STM32F101ZE */ 
  /* #define STM32F101xG  */   /*!< STM32F101RF, STM32F101VF, STM32F101ZF, STM32F101RG, STM32F101VG and STM32F101ZG */
  /* #define STM32F102x6 */    /*!< STM32F102C4, STM32F102R4, STM32F102C6 and STM32F102R6 */
  /* #define STM32F102xB  */   /*!< STM32F102C8, STM32F102R8, STM32F102CB and STM32F102RB */
  /* #define STM32F103x6  */   /*!< STM32F103C4, STM32F103R4, STM32F103T4, STM32F103C6, STM32F103R6 and STM32F103T6 */
  /* #define STM32F103xB  */   /*!< STM32F103C8, STM32F103R8, STM32F103T8, STM32F103V8, STM32F103CB, STM32F103RB, STM32F103TB and STM32F103VB */
  /* #define STM32F103xE */    /*!< STM32F103RC, STM32F103VC, STM32F103ZC, STM32F103RD, STM32F103VD, STM32F103ZD, STM32F103RE, STM32F103VE and STM32F103ZE */
  /* #define STM32F103xG  */   /*!< STM32F103RF, STM32F103VF, STM32F103ZF, STM32F103RG, STM32F103VG and STM32F103ZG */
  /* #define STM32F105xC */    /*!< STM32F105R8, STM32F105V8, STM32F105RB, STM32F105VB, STM32F105RC and STM32F105VC */
  /* #define STM32F107xC  */   /*!< STM32F107RB, STM32F107VB, STM32F107RC and STM32F107VC */  
#endif

/*  Tip: To avoid modifying this file each time you need to switch between these
        devices, you can define the device in your toolchain compiler preprocessor.
  */
  
#if !defined  (USE_HAL_DRIVER)
/**
 * @brief Comment the line below if you will not use the peripherals drivers.
   In this case, these drivers will not be included and the application code will 
   be based on direct access to peripherals registers 
   */
  /*#define USE_HAL_DRIVER */
#endif /* USE_HAL_DRIVER */

/**
  * @brief CMSIS Device version number V4.3.1
  */
#define __STM32F1_CMSIS_VERSION_MAIN   (0x04) /*!< [31:24] main version */
#define __STM32F1_CMSIS_VERSION_SUB1   (0x03) /*!< [23:16] sub1 version */
#define __STM32F1_CMSIS_VERSION_SUB2   (0x01) /*!< [15:8]  sub2 version */
#define __STM32F1_CMSIS_VERSION_RC     (0x00) /*!< [7:0]  release candidate */ 
#define __STM32F1_CMSIS_VERSION        ((__STM32F1_CMSIS_VERSION_MAIN << 24)\
                                       |(__STM32F1_CMSIS_VERSION_SUB1 << 16)\
                                       |(__STM32F1_CMSIS_VERSION_SUB2 << 8 )\
                                       |(__STM32F1_CMSIS_VERSION_RC))

/**
  * @}
  */

/** @addtogroup Device_Included
  * @{
  */

#if defined(STM32F100xB)
  #include "stm32f100xb.h"
#elif defined(STM32F100xE)
  #include "stm32f100xe.h"
#elif defined(STM32F101x6)
  #include "stm32f101x6.h"
#elif defined(STM32F101xB)
  #include "stm32f101xb.h"
#elif defined(STM32F101xE)
  #include "stm32f101xe.h"
#elif defined(STM32F101xG)
  #include "stm32f101xg.h"
#elif defined(STM32F102x6)
  #include "stm32f102x6.h"
#elif defined(STM32F102xB)
  #include "stm32f102xb.h"
#elif defined(STM32F103x6)
  #include "stm32f103x6.h"
#elif defined(STM32F103xB)
  #include "stm32f103xb.h"
#elif defined(STM32F103xE)
  #include "stm32f103xe.h"
#elif defined(STM32F103xG)
  #include "stm32f103xg.h"
#elif defined(STM32F105xC)
  #include "stm32f105xc.h"
#elif defined(STM32F107xC)
  #include "stm32f107xc.h"
#else
 #error "Please select first the target STM32F1xx device used in your application (in stm32f1xx.h file)"
#endif

/**
  * @}
  */

/** @addtogroup Exported_types
  * @{
  */  
typedef enum 
{
  RESET = 0, 
  SET = !RESET
} FlagStatus, ITStatus;

typedef enum 
{
  DISABLE = 0, 
  ENABLE = !DISABLE
} FunctionalState;
#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

typedef enum
{
  SUCCESS = 0U,
  ST_ERROR = !SUCCESS
} ErrorStatus;

/**
  * @}
  */


/** @addtogroup Exported_macros
  * @{
  */
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define CLEAR_REG(REG)        ((REG) = (0x0))

#define WRITE_REG(REG, VAL)   ((REG) = (VAL))

#define READ_REG(REG)         ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

#define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL))) 


/**
  * @}
  */

#if defined (USE_HAL_DRIVER)
 #include <stm32f1xx_hal.h>
#endif /* USE_HAL_DRIVER */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STM32F1xx_H */
/**
  * @}
  */

/**
  * @}
  */
  



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/include/stm32f103xb.h`

```c

/* excerpt lines 1-60 */
/**
  ******************************************************************************
  * @file    stm32f103xb.h
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M3 Device Peripheral Access Layer Header File. 
  *          This file contains all the peripheral register's definitions, bits 
  *          definitions and memory mapping for STM32F1xx devices.            
  *            
  *          This file contains:
  *           - Data structures and the address mapping for all peripherals
  *           - Peripheral's registers declarations and bits definition
  *           - Macros to access peripheral�s registers hardware
  *  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */


/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32f103xb
  * @{
  */
    
#ifndef __STM32F103xB_H
#define __STM32F103xB_H

#ifdef __cplusplus
 extern "C" {
#endif 

/** @addtogroup Configuration_section_for_CMSIS
  * @{
  */
/**
  * @brief Configuration of the Cortex-M3 Processor and Core Peripherals 
 */
#define __CM3_REV                  0x0200U  /*!< Core Revision r2p0                           */
 #define __MPU_PRESENT             0U       /*!< Other STM32 devices does not provide an MPU  */
#define __NVIC_PRIO_BITS           4U       /*!< STM32 uses 4 Bits for the Priority Levels    */
#define __Vendor_SysTickConfig     0U       /*!< Set to 1 if different SysTick Config is used */

/**
  * @}
  */

/** @addtogroup Peripheral_interrupt_number_definition
  * @{

/* excerpt lines 102-150 */
  USB_HP_CAN1_TX_IRQn         = 19,     /*!< USB Device High Priority or CAN1 TX Interrupts       */
  USB_LP_CAN1_RX0_IRQn        = 20,     /*!< USB Device Low Priority or CAN1 RX0 Interrupts       */
  CAN1_RX1_IRQn               = 21,     /*!< CAN1 RX1 Interrupt                                   */
  CAN1_SCE_IRQn               = 22,     /*!< CAN1 SCE Interrupt                                   */
  EXTI9_5_IRQn                = 23,     /*!< External Line[9:5] Interrupts                        */
  TIM1_BRK_IRQn               = 24,     /*!< TIM1 Break Interrupt                                 */
  TIM1_UP_IRQn                = 25,     /*!< TIM1 Update Interrupt                                */
  TIM1_TRG_COM_IRQn           = 26,     /*!< TIM1 Trigger and Commutation Interrupt               */
  TIM1_CC_IRQn                = 27,     /*!< TIM1 Capture Compare Interrupt                       */
  TIM2_IRQn                   = 28,     /*!< TIM2 global Interrupt                                */
  TIM3_IRQn                   = 29,     /*!< TIM3 global Interrupt                                */
  TIM4_IRQn                   = 30,     /*!< TIM4 global Interrupt                                */
  I2C1_EV_IRQn                = 31,     /*!< I2C1 Event Interrupt                                 */
  I2C1_ER_IRQn                = 32,     /*!< I2C1 Error Interrupt                                 */
  I2C2_EV_IRQn                = 33,     /*!< I2C2 Event Interrupt                                 */
  I2C2_ER_IRQn                = 34,     /*!< I2C2 Error Interrupt                                 */
  SPI1_IRQn                   = 35,     /*!< SPI1 global Interrupt                                */
  SPI2_IRQn                   = 36,     /*!< SPI2 global Interrupt                                */
  USART1_IRQn                 = 37,     /*!< USART1 global Interrupt                              */
  USART2_IRQn                 = 38,     /*!< USART2 global Interrupt                              */
  USART3_IRQn                 = 39,     /*!< USART3 global Interrupt                              */
  EXTI15_10_IRQn              = 40,     /*!< External Line[15:10] Interrupts                      */
  RTC_Alarm_IRQn              = 41,     /*!< RTC Alarm through EXTI Line Interrupt                */
  USBWakeUp_IRQn              = 42,     /*!< USB Device WakeUp from suspend through EXTI Line Interrupt */
} IRQn_Type;

/**
  * @}
  */

#include "core_cm3.h"
#include "system_stm32f1xx.h"
#include <stdint.h>

/** @addtogroup Peripheral_registers_structures
  * @{
  */   

/** 
  * @brief Analog to Digital Converter  
  */

typedef struct
{
  __IO uint32_t SR;
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t SMPR1;
  __IO uint32_t SMPR2;

/* excerpt lines 456-503 */
} RTC_TypeDef;

/** 
  * @brief Serial Peripheral Interface
  */

typedef struct
{
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t SR;
  __IO uint32_t DR;
  __IO uint32_t CRCPR;
  __IO uint32_t RXCRCR;
  __IO uint32_t TXCRCR;
  __IO uint32_t I2SCFGR;
} SPI_TypeDef;

/**
  * @brief TIM Timers
  */
typedef struct
{
  __IO uint32_t CR1;             /*!< TIM control register 1,                      Address offset: 0x00 */
  __IO uint32_t CR2;             /*!< TIM control register 2,                      Address offset: 0x04 */
  __IO uint32_t SMCR;            /*!< TIM slave Mode Control register,             Address offset: 0x08 */
  __IO uint32_t DIER;            /*!< TIM DMA/interrupt enable register,           Address offset: 0x0C */
  __IO uint32_t SR;              /*!< TIM status register,                         Address offset: 0x10 */
  __IO uint32_t EGR;             /*!< TIM event generation register,               Address offset: 0x14 */
  __IO uint32_t CCMR1;           /*!< TIM  capture/compare mode register 1,        Address offset: 0x18 */
  __IO uint32_t CCMR2;           /*!< TIM  capture/compare mode register 2,        Address offset: 0x1C */
  __IO uint32_t CCER;            /*!< TIM capture/compare enable register,         Address offset: 0x20 */
  __IO uint32_t CNT;             /*!< TIM counter register,                        Address offset: 0x24 */
  __IO uint32_t PSC;             /*!< TIM prescaler register,                      Address offset: 0x28 */
  __IO uint32_t ARR;             /*!< TIM auto-reload register,                    Address offset: 0x2C */
  __IO uint32_t RCR;             /*!< TIM  repetition counter register,            Address offset: 0x30 */
  __IO uint32_t CCR1;            /*!< TIM capture/compare register 1,              Address offset: 0x34 */
  __IO uint32_t CCR2;            /*!< TIM capture/compare register 2,              Address offset: 0x38 */
  __IO uint32_t CCR3;            /*!< TIM capture/compare register 3,              Address offset: 0x3C */
  __IO uint32_t CCR4;            /*!< TIM capture/compare register 4,              Address offset: 0x40 */
  __IO uint32_t BDTR;            /*!< TIM break and dead-time register,            Address offset: 0x44 */
  __IO uint32_t DCR;             /*!< TIM DMA control register,                    Address offset: 0x48 */
  __IO uint32_t DMAR;            /*!< TIM DMA address for full transfer register,  Address offset: 0x4C */
  __IO uint32_t OR;              /*!< TIM option register,                         Address offset: 0x50 */
}TIM_TypeDef;


/** 

/* excerpt lines 577-705 */

#define SRAM_BB_BASE          0x22000000UL /*!< SRAM base address in the bit-band region */
#define PERIPH_BB_BASE        0x42000000UL /*!< Peripheral base address in the bit-band region */


/*!< Peripheral memory map */
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000UL)

#define TIM2_BASE             (APB1PERIPH_BASE + 0x00000000UL)
#define TIM3_BASE             (APB1PERIPH_BASE + 0x00000400UL)
#define TIM4_BASE             (APB1PERIPH_BASE + 0x00000800UL)
#define RTC_BASE              (APB1PERIPH_BASE + 0x00002800UL)
#define WWDG_BASE             (APB1PERIPH_BASE + 0x00002C00UL)
#define IWDG_BASE             (APB1PERIPH_BASE + 0x00003000UL)
#define SPI2_BASE             (APB1PERIPH_BASE + 0x00003800UL)
#define USART2_BASE           (APB1PERIPH_BASE + 0x00004400UL)
#define USART3_BASE           (APB1PERIPH_BASE + 0x00004800UL)
#define I2C1_BASE             (APB1PERIPH_BASE + 0x00005400UL)
#define I2C2_BASE             (APB1PERIPH_BASE + 0x00005800UL)
#define CAN1_BASE             (APB1PERIPH_BASE + 0x00006400UL)
#define BKP_BASE              (APB1PERIPH_BASE + 0x00006C00UL)
#define PWR_BASE              (APB1PERIPH_BASE + 0x00007000UL)
#define AFIO_BASE             (APB2PERIPH_BASE + 0x00000000UL)
#define EXTI_BASE             (APB2PERIPH_BASE + 0x00000400UL)
#define GPIOA_BASE            (APB2PERIPH_BASE + 0x00000800UL)
#define GPIOB_BASE            (APB2PERIPH_BASE + 0x00000C00UL)
#define GPIOC_BASE            (APB2PERIPH_BASE + 0x00001000UL)
#define GPIOD_BASE            (APB2PERIPH_BASE + 0x00001400UL)
#define GPIOE_BASE            (APB2PERIPH_BASE + 0x00001800UL)
#define ADC1_BASE             (APB2PERIPH_BASE + 0x00002400UL)
#define ADC2_BASE             (APB2PERIPH_BASE + 0x00002800UL)
#define TIM1_BASE             (APB2PERIPH_BASE + 0x00002C00UL)
#define SPI1_BASE             (APB2PERIPH_BASE + 0x00003000UL)
#define USART1_BASE           (APB2PERIPH_BASE + 0x00003800UL)


#define DMA1_BASE             (AHBPERIPH_BASE + 0x00000000UL)
#define DMA1_Channel1_BASE    (AHBPERIPH_BASE + 0x00000008UL)
#define DMA1_Channel2_BASE    (AHBPERIPH_BASE + 0x0000001CUL)
#define DMA1_Channel3_BASE    (AHBPERIPH_BASE + 0x00000030UL)
#define DMA1_Channel4_BASE    (AHBPERIPH_BASE + 0x00000044UL)
#define DMA1_Channel5_BASE    (AHBPERIPH_BASE + 0x00000058UL)
#define DMA1_Channel6_BASE    (AHBPERIPH_BASE + 0x0000006CUL)
#define DMA1_Channel7_BASE    (AHBPERIPH_BASE + 0x00000080UL)
#define RCC_BASE              (AHBPERIPH_BASE + 0x00001000UL)
#define CRC_BASE              (AHBPERIPH_BASE + 0x00003000UL)

#define FLASH_R_BASE          (AHBPERIPH_BASE + 0x00002000UL) /*!< Flash registers base address */
#define FLASHSIZE_BASE        0x1FFFF7E0UL    /*!< FLASH Size register base address */
#define UID_BASE              0x1FFFF7E8UL    /*!< Unique device ID register base address */
#define OB_BASE               0x1FFFF800UL    /*!< Flash Option Bytes base address */



#define DBGMCU_BASE          0xE0042000UL /*!< Debug MCU registers base address */

/* USB device FS */
#define USB_BASE              (APB1PERIPH_BASE + 0x00005C00UL) /*!< USB_IP Peripheral Registers base address */
#define USB_PMAADDR           (APB1PERIPH_BASE + 0x00006000UL) /*!< USB_IP Packet Memory Area base address */


/**
  * @}
  */
  
/** @addtogroup Peripheral_declaration
  * @{
  */  

#define TIM2                ((TIM_TypeDef *)TIM2_BASE)
#define TIM3                ((TIM_TypeDef *)TIM3_BASE)
#define TIM4                ((TIM_TypeDef *)TIM4_BASE)
#define RTC                 ((RTC_TypeDef *)RTC_BASE)
#define WWDG                ((WWDG_TypeDef *)WWDG_BASE)
#define IWDG                ((IWDG_TypeDef *)IWDG_BASE)
#define SPI2                ((SPI_TypeDef *)SPI2_BASE)
#define USART2              ((USART_TypeDef *)USART2_BASE)
#define USART3              ((USART_TypeDef *)USART3_BASE)
#define I2C1                ((I2C_TypeDef *)I2C1_BASE)
#define I2C2                ((I2C_TypeDef *)I2C2_BASE)
#define USB                 ((USB_TypeDef *)USB_BASE)
#define CAN1                ((CAN_TypeDef *)CAN1_BASE)
#define BKP                 ((BKP_TypeDef *)BKP_BASE)
#define PWR                 ((PWR_TypeDef *)PWR_BASE)
#define AFIO                ((AFIO_TypeDef *)AFIO_BASE)
#define EXTI                ((EXTI_TypeDef *)EXTI_BASE)
#define GPIOA               ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *)GPIOE_BASE)
#define ADC1                ((ADC_TypeDef *)ADC1_BASE)
#define ADC2                ((ADC_TypeDef *)ADC2_BASE)
#define ADC12_COMMON        ((ADC_Common_TypeDef *)ADC1_BASE)
#define TIM1                ((TIM_TypeDef *)TIM1_BASE)
#define SPI1                ((SPI_TypeDef *)SPI1_BASE)
#define USART1              ((USART_TypeDef *)USART1_BASE)
#define DMA1                ((DMA_TypeDef *)DMA1_BASE)
#define DMA1_Channel1       ((DMA_Channel_TypeDef *)DMA1_Channel1_BASE)
#define DMA1_Channel2       ((DMA_Channel_TypeDef *)DMA1_Channel2_BASE)
#define DMA1_Channel3       ((DMA_Channel_TypeDef *)DMA1_Channel3_BASE)
#define DMA1_Channel4       ((DMA_Channel_TypeDef *)DMA1_Channel4_BASE)
#define DMA1_Channel5       ((DMA_Channel_TypeDef *)DMA1_Channel5_BASE)
#define DMA1_Channel6       ((DMA_Channel_TypeDef *)DMA1_Channel6_BASE)
#define DMA1_Channel7       ((DMA_Channel_TypeDef *)DMA1_Channel7_BASE)
#define RCC                 ((RCC_TypeDef *)RCC_BASE)
#define CRC                 ((CRC_TypeDef *)CRC_BASE)
#define FLASH               ((FLASH_TypeDef *)FLASH_R_BASE)
#define OB                  ((OB_TypeDef *)OB_BASE)
#define DBGMCU              ((DBGMCU_TypeDef *)DBGMCU_BASE)


/**
  * @}
  */

/** @addtogroup Exported_constants
  * @{
  */
  
  /** @addtogroup Peripheral_Registers_Bits_Definition
  * @{
  */
    
/******************************************************************************/
/*                         Peripheral Registers_Bits_Definition               */
/******************************************************************************/

/* excerpt lines 1172-1267 */
#define RCC_APB2RSTR_IOPCRST_Msk             (0x1UL << RCC_APB2RSTR_IOPCRST_Pos) /*!< 0x00000010 */
#define RCC_APB2RSTR_IOPCRST                 RCC_APB2RSTR_IOPCRST_Msk          /*!< I/O port C reset */
#define RCC_APB2RSTR_IOPDRST_Pos             (5U)                              
#define RCC_APB2RSTR_IOPDRST_Msk             (0x1UL << RCC_APB2RSTR_IOPDRST_Pos) /*!< 0x00000020 */
#define RCC_APB2RSTR_IOPDRST                 RCC_APB2RSTR_IOPDRST_Msk          /*!< I/O port D reset */
#define RCC_APB2RSTR_ADC1RST_Pos             (9U)                              
#define RCC_APB2RSTR_ADC1RST_Msk             (0x1UL << RCC_APB2RSTR_ADC1RST_Pos) /*!< 0x00000200 */
#define RCC_APB2RSTR_ADC1RST                 RCC_APB2RSTR_ADC1RST_Msk          /*!< ADC 1 interface reset */

#define RCC_APB2RSTR_ADC2RST_Pos             (10U)                             
#define RCC_APB2RSTR_ADC2RST_Msk             (0x1UL << RCC_APB2RSTR_ADC2RST_Pos) /*!< 0x00000400 */
#define RCC_APB2RSTR_ADC2RST                 RCC_APB2RSTR_ADC2RST_Msk          /*!< ADC 2 interface reset */

#define RCC_APB2RSTR_TIM1RST_Pos             (11U)                             
#define RCC_APB2RSTR_TIM1RST_Msk             (0x1UL << RCC_APB2RSTR_TIM1RST_Pos) /*!< 0x00000800 */
#define RCC_APB2RSTR_TIM1RST                 RCC_APB2RSTR_TIM1RST_Msk          /*!< TIM1 Timer reset */
#define RCC_APB2RSTR_SPI1RST_Pos             (12U)                             
#define RCC_APB2RSTR_SPI1RST_Msk             (0x1UL << RCC_APB2RSTR_SPI1RST_Pos) /*!< 0x00001000 */
#define RCC_APB2RSTR_SPI1RST                 RCC_APB2RSTR_SPI1RST_Msk          /*!< SPI 1 reset */
#define RCC_APB2RSTR_USART1RST_Pos           (14U)                             
#define RCC_APB2RSTR_USART1RST_Msk           (0x1UL << RCC_APB2RSTR_USART1RST_Pos) /*!< 0x00004000 */
#define RCC_APB2RSTR_USART1RST               RCC_APB2RSTR_USART1RST_Msk        /*!< USART1 reset */


#define RCC_APB2RSTR_IOPERST_Pos             (6U)                              
#define RCC_APB2RSTR_IOPERST_Msk             (0x1UL << RCC_APB2RSTR_IOPERST_Pos) /*!< 0x00000040 */
#define RCC_APB2RSTR_IOPERST                 RCC_APB2RSTR_IOPERST_Msk          /*!< I/O port E reset */




/*****************  Bit definition for RCC_APB1RSTR register  *****************/
#define RCC_APB1RSTR_TIM2RST_Pos             (0U)                              
#define RCC_APB1RSTR_TIM2RST_Msk             (0x1UL << RCC_APB1RSTR_TIM2RST_Pos) /*!< 0x00000001 */
#define RCC_APB1RSTR_TIM2RST                 RCC_APB1RSTR_TIM2RST_Msk          /*!< Timer 2 reset */
#define RCC_APB1RSTR_TIM3RST_Pos             (1U)                              
#define RCC_APB1RSTR_TIM3RST_Msk             (0x1UL << RCC_APB1RSTR_TIM3RST_Pos) /*!< 0x00000002 */
#define RCC_APB1RSTR_TIM3RST                 RCC_APB1RSTR_TIM3RST_Msk          /*!< Timer 3 reset */
#define RCC_APB1RSTR_WWDGRST_Pos             (11U)                             
#define RCC_APB1RSTR_WWDGRST_Msk             (0x1UL << RCC_APB1RSTR_WWDGRST_Pos) /*!< 0x00000800 */
#define RCC_APB1RSTR_WWDGRST                 RCC_APB1RSTR_WWDGRST_Msk          /*!< Window Watchdog reset */
#define RCC_APB1RSTR_USART2RST_Pos           (17U)                             
#define RCC_APB1RSTR_USART2RST_Msk           (0x1UL << RCC_APB1RSTR_USART2RST_Pos) /*!< 0x00020000 */
#define RCC_APB1RSTR_USART2RST               RCC_APB1RSTR_USART2RST_Msk        /*!< USART 2 reset */
#define RCC_APB1RSTR_I2C1RST_Pos             (21U)                             
#define RCC_APB1RSTR_I2C1RST_Msk             (0x1UL << RCC_APB1RSTR_I2C1RST_Pos) /*!< 0x00200000 */
#define RCC_APB1RSTR_I2C1RST                 RCC_APB1RSTR_I2C1RST_Msk          /*!< I2C 1 reset */

#define RCC_APB1RSTR_CAN1RST_Pos             (25U)                             
#define RCC_APB1RSTR_CAN1RST_Msk             (0x1UL << RCC_APB1RSTR_CAN1RST_Pos) /*!< 0x02000000 */
#define RCC_APB1RSTR_CAN1RST                 RCC_APB1RSTR_CAN1RST_Msk          /*!< CAN1 reset */

#define RCC_APB1RSTR_BKPRST_Pos              (27U)                             
#define RCC_APB1RSTR_BKPRST_Msk              (0x1UL << RCC_APB1RSTR_BKPRST_Pos) /*!< 0x08000000 */
#define RCC_APB1RSTR_BKPRST                  RCC_APB1RSTR_BKPRST_Msk           /*!< Backup interface reset */
#define RCC_APB1RSTR_PWRRST_Pos              (28U)                             
#define RCC_APB1RSTR_PWRRST_Msk              (0x1UL << RCC_APB1RSTR_PWRRST_Pos) /*!< 0x10000000 */
#define RCC_APB1RSTR_PWRRST                  RCC_APB1RSTR_PWRRST_Msk           /*!< Power interface reset */

#define RCC_APB1RSTR_TIM4RST_Pos             (2U)                              
#define RCC_APB1RSTR_TIM4RST_Msk             (0x1UL << RCC_APB1RSTR_TIM4RST_Pos) /*!< 0x00000004 */
#define RCC_APB1RSTR_TIM4RST                 RCC_APB1RSTR_TIM4RST_Msk          /*!< Timer 4 reset */
#define RCC_APB1RSTR_SPI2RST_Pos             (14U)                             
#define RCC_APB1RSTR_SPI2RST_Msk             (0x1UL << RCC_APB1RSTR_SPI2RST_Pos) /*!< 0x00004000 */
#define RCC_APB1RSTR_SPI2RST                 RCC_APB1RSTR_SPI2RST_Msk          /*!< SPI 2 reset */
#define RCC_APB1RSTR_USART3RST_Pos           (18U)                             
#define RCC_APB1RSTR_USART3RST_Msk           (0x1UL << RCC_APB1RSTR_USART3RST_Pos) /*!< 0x00040000 */
#define RCC_APB1RSTR_USART3RST               RCC_APB1RSTR_USART3RST_Msk        /*!< USART 3 reset */
#define RCC_APB1RSTR_I2C2RST_Pos             (22U)                             
#define RCC_APB1RSTR_I2C2RST_Msk             (0x1UL << RCC_APB1RSTR_I2C2RST_Pos) /*!< 0x00400000 */
#define RCC_APB1RSTR_I2C2RST                 RCC_APB1RSTR_I2C2RST_Msk          /*!< I2C 2 reset */

#define RCC_APB1RSTR_USBRST_Pos              (23U)                             
#define RCC_APB1RSTR_USBRST_Msk              (0x1UL << RCC_APB1RSTR_USBRST_Pos) /*!< 0x00800000 */
#define RCC_APB1RSTR_USBRST                  RCC_APB1RSTR_USBRST_Msk           /*!< USB Device reset */






/******************  Bit definition for RCC_AHBENR register  ******************/
#define RCC_AHBENR_DMA1EN_Pos                (0U)                              
#define RCC_AHBENR_DMA1EN_Msk                (0x1UL << RCC_AHBENR_DMA1EN_Pos)   /*!< 0x00000001 */
#define RCC_AHBENR_DMA1EN                    RCC_AHBENR_DMA1EN_Msk             /*!< DMA1 clock enable */
#define RCC_AHBENR_SRAMEN_Pos                (2U)                              
#define RCC_AHBENR_SRAMEN_Msk                (0x1UL << RCC_AHBENR_SRAMEN_Pos)   /*!< 0x00000004 */
#define RCC_AHBENR_SRAMEN                    RCC_AHBENR_SRAMEN_Msk             /*!< SRAM interface clock enable */
#define RCC_AHBENR_FLITFEN_Pos               (4U)                              
#define RCC_AHBENR_FLITFEN_Msk               (0x1UL << RCC_AHBENR_FLITFEN_Pos)  /*!< 0x00000010 */
#define RCC_AHBENR_FLITFEN                   RCC_AHBENR_FLITFEN_Msk            /*!< FLITF clock enable */
#define RCC_AHBENR_CRCEN_Pos                 (6U)                              
#define RCC_AHBENR_CRCEN_Msk                 (0x1UL << RCC_AHBENR_CRCEN_Pos)    /*!< 0x00000040 */
#define RCC_AHBENR_CRCEN                     RCC_AHBENR_CRCEN_Msk              /*!< CRC clock enable */



/* ... additional content omitted by deterministic excerpt limit ... */
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/include/system_stm32f1xx.h`

```c
/**
  ******************************************************************************
  * @file    system_stm32f10x.h
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M3 Device Peripheral Access Layer System Header File.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32f10x_system
  * @{
  */  
  
/**
  * @brief Define to prevent recursive inclusion
  */
#ifndef __SYSTEM_STM32F10X_H
#define __SYSTEM_STM32F10X_H

#ifdef __cplusplus
 extern "C" {
#endif 

/** @addtogroup STM32F10x_System_Includes
  * @{
  */

/**
  * @}
  */


/** @addtogroup STM32F10x_System_Exported_types
  * @{
  */

extern uint32_t SystemCoreClock;          /*!< System Clock Frequency (Core Clock) */
extern const uint8_t  AHBPrescTable[16U];  /*!< AHB prescalers table values */
extern const uint8_t  APBPrescTable[8U];   /*!< APB prescalers table values */

/**
  * @}
  */

/** @addtogroup STM32F10x_System_Exported_Constants
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32F10x_System_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32F10x_System_Exported_Functions
  * @{
  */
  
extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);
/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /*__SYSTEM_STM32F10X_H */

/**
  * @}
  */
  
/**
  * @}
  */  
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/inc/stm32f1xx_hal_conf.h`

```c
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_conf.h
  * @brief   HAL configuration file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2018 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* ########################## Module Selection ############################## */
/**
  * @brief This is the list of modules to be used in the HAL driver 
  */
  
#define HAL_MODULE_ENABLED  
/*#define HAL_ADC_MODULE_ENABLED   */
/*#define HAL_CRYP_MODULE_ENABLED   */
#define HAL_CAN_MODULE_ENABLED
/*#define HAL_CEC_MODULE_ENABLED   */
/*#define HAL_CORTEX_MODULE_ENABLED   */
/*#define HAL_CRC_MODULE_ENABLED   */
/*#define HAL_DAC_MODULE_ENABLED   */
/*#define HAL_DMA_MODULE_ENABLED   */
/*#define HAL_ETH_MODULE_ENABLED   */
/*#define HAL_FLASH_MODULE_ENABLED   */
#define HAL_GPIO_MODULE_ENABLED
/*#define HAL_I2C_MODULE_ENABLED   */
/*#define HAL_I2S_MODULE_ENABLED   */
/*#define HAL_IRDA_MODULE_ENABLED   */
/*#define HAL_IWDG_MODULE_ENABLED   */
/*#define HAL_NOR_MODULE_ENABLED   */
/*#define HAL_NAND_MODULE_ENABLED   */
/*#define HAL_PCCARD_MODULE_ENABLED   */
/*#define HAL_PCD_MODULE_ENABLED   */
/*#define HAL_HCD_MODULE_ENABLED   */
/*#define HAL_PWR_MODULE_ENABLED   */
/*#define HAL_RCC_MODULE_ENABLED   */
/*#define HAL_RTC_MODULE_ENABLED   */
/*#define HAL_SD_MODULE_ENABLED   */
/*#define HAL_MMC_MODULE_ENABLED   */
/*#define HAL_SDRAM_MODULE_ENABLED   */
/*#define HAL_SMARTCARD_MODULE_ENABLED   */
/*#define HAL_SPI_MODULE_ENABLED   */
/*#define HAL_SRAM_MODULE_ENABLED   */
/*#define HAL_TIM_MODULE_ENABLED   */
#define HAL_UART_MODULE_ENABLED
/*#define HAL_USART_MODULE_ENABLED   */
/*#define HAL_WWDG_MODULE_ENABLED   */
/*#define HAL_EXTI_MODULE_ENABLED   */

#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED

/* ########################## Oscillator Values adaptation ####################*/
/**
  * @brief Adjust the value of External High Speed oscillator (HSE) used in your application.
  *        This value is used by the RCC HAL module to compute the system frequency
  *        (when HSE is used as system clock source, directly or through the PLL).  
  */
#if !defined  (HSE_VALUE) 
  #define HSE_VALUE    ((uint32_t)8000000) /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSE_STARTUP_TIMEOUT)
  #define HSE_STARTUP_TIMEOUT    ((uint32_t)100)   /*!< Time out for HSE start up, in ms */
#endif /* HSE_STARTUP_TIMEOUT */

/**
  * @brief Internal High Speed oscillator (HSI) value.
  *        This value is used by the RCC HAL module to compute the system frequency
  *        (when HSI is used as system clock source, directly or through the PLL). 
  */
#if !defined  (HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)8000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

/**
  * @brief Internal Low Speed oscillator (LSI) value.
  */
#if !defined  (LSI_VALUE) 
 #define LSI_VALUE               40000U    /*!< LSI Typical Value in Hz */
#endif /* LSI_VALUE */                     /*!< Value of the Internal Low Speed oscillator in Hz
                                                The real value may vary depending on the variations
                                                in voltage and temperature. */

/**
  * @brief External Low Speed oscillator (LSE) value.
  *        This value is used by the UART, RTC HAL module to compute the system frequency
  */
#if !defined  (LSE_VALUE)
  #define LSE_VALUE    ((uint32_t)32768) /*!< Value of the External oscillator in Hz*/
#endif /* LSE_VALUE */

#if !defined  (LSE_STARTUP_TIMEOUT)
  #define LSE_STARTUP_TIMEOUT    ((uint32_t)5000)   /*!< Time out for LSE start up, in ms */
#endif /* LSE_STARTUP_TIMEOUT */

/* Tip: To avoid modifying this file each time you need to use different HSE,
   ===  you can define the HSE value in your toolchain compiler preprocessor. */

/* ########################### System Configuration ######################### */
/**
  * @brief This is the HAL system configuration section
  */     
#define  VDD_VALUE                    ((uint32_t)3300) /*!< Value of VDD in mv */           
#define  TICK_INT_PRIORITY            ((uint32_t)0)    /*!< tick interrupt priority (lowest by default)  */            
#define  USE_RTOS                     0
#define  PREFETCH_ENABLE              1

/* ########################## Assert Selection ############################## */
/**
  * @brief Uncomment the line below to expanse the "assert_param" macro in the 
  *        HAL drivers code
  */
/* #define USE_FULL_ASSERT    1U */

/* ################## Ethernet peripheral configuration ##################### */

/* Section 1 : Ethernet peripheral configuration */

/* MAC ADDRESS: MAC_ADDR0:MAC_ADDR1:MAC_ADDR2:MAC_ADDR3:MAC_ADDR4:MAC_ADDR5 */
#define MAC_ADDR0   2
#define MAC_ADDR1   0
#define MAC_ADDR2   0
#define MAC_ADDR3   0
#define MAC_ADDR4   0
#define MAC_ADDR5   0

/* Definition of the Ethernet driver buffers size and count */   
#define ETH_RX_BUF_SIZE                ETH_MAX_PACKET_SIZE /* buffer size for receive               */
#define ETH_TX_BUF_SIZE                ETH_MAX_PACKET_SIZE /* buffer size for transmit              */
#define ETH_RXBUFNB                    ((uint32_t)8)       /* 4 Rx buffers of size ETH_RX_BUF_SIZE  */
#define ETH_TXBUFNB                    ((uint32_t)4)       /* 4 Tx buffers of size ETH_TX_BUF_SIZE  */

/* Section 2: PHY configuration section */

/* DP83848_PHY_ADDRESS Address*/ 
#define DP83848_PHY_ADDRESS           0x01U
/* PHY Reset delay these values are based on a 1 ms Systick interrupt*/ 
#define PHY_RESET_DELAY                 ((uint32_t)0x000000FF)
/* PHY Configuration delay */
#define PHY_CONFIG_DELAY                ((uint32_t)0x00000FFF)

#define PHY_READ_TO                     ((uint32_t)0x0000FFFF)
#define PHY_WRITE_TO                    ((uint32_t)0x0000FFFF)

/* Section 3: Common PHY Registers */

#define PHY_BCR                         ((uint16_t)0x00)    /*!< Transceiver Basic Control Register   */
#define PHY_BSR                         ((uint16_t)0x01)    /*!< Transceiver Basic Status Register    */
 
#define PHY_RESET                       ((uint16_t)0x8000)  /*!< PHY Reset */
#define PHY_LOOPBACK                    ((uint16_t)0x4000)  /*!< Select loop-back mode */
#define PHY_FULLDUPLEX_100M             ((uint16_t)0x2100)  /*!< Set the full-duplex mode at 100 Mb/s */
#define PHY_HALFDUPLEX_100M             ((uint16_t)0x2000)  /*!< Set the half-duplex mode at 100 Mb/s */
#define PHY_FULLDUPLEX_10M              ((uint16_t)0x0100)  /*!< Set the full-duplex mode at 10 Mb/s  */
#define PHY_HALFDUPLEX_10M              ((uint16_t)0x0000)  /*!< Set the half-duplex mode at 10 Mb/s  */
#define PHY_AUTONEGOTIATION             ((uint16_t)0x1000)  /*!< Enable auto-negotiation function     */
#define PHY_RESTART_AUTONEGOTIATION     ((uint16_t)0x0200)  /*!< Restart auto-negotiation function    */
#define PHY_POWERDOWN                   ((uint16_t)0x0800)  /*!< Select the power down mode           */
#define PHY_ISOLATE                     ((uint16_t)0x0400)  /*!< Isolate PHY from MII                 */

#define PHY_AUTONEGO_COMPLETE           ((uint16_t)0x0020)  /*!< Auto-Negotiation process completed   */
#define PHY_LINKED_STATUS               ((uint16_t)0x0004)  /*!< Valid link established               */
#define PHY_JABBER_DETECTION            ((uint16_t)0x0002)  /*!< Jabber condition detected            */
  
/* Section 4: Extended PHY Registers */
#define PHY_SR                          ((uint16_t)0x10U)    /*!< PHY status register Offset                      */

#define PHY_SPEED_STATUS                ((uint16_t)0x0002U)  /*!< PHY Speed mask                                  */
#define PHY_DUPLEX_STATUS               ((uint16_t)0x0004U)  /*!< PHY Duplex mask                                 */

/* Includes ------------------------------------------------------------------*/
/**
  * @brief Include module's header file 
  */

#ifdef HAL_RCC_MODULE_ENABLED
 #include "stm32f1xx_hal_rcc.h"
#endif /* HAL_RCC_MODULE_ENABLED */

#ifdef HAL_EXTI_MODULE_ENABLED
 #include "stm32f1xx_hal_exti.h"
#endif /* HAL_EXTI_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
 #include "stm32f1xx_hal_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */
   
#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f1xx_hal_dma.h"
#endif /* HAL_DMA_MODULE_ENABLED */
   
#ifdef HAL_ETH_MODULE_ENABLED
  #include "stm32f1xx_hal_eth.h"
#endif /* HAL_ETH_MODULE_ENABLED */  
   
#ifdef HAL_CAN_MODULE_ENABLED
 #include "stm32f1xx_hal_can.h"
#endif /* HAL_CAN_MODULE_ENABLED */

#ifdef HAL_CEC_MODULE_ENABLED
 #include "stm32f1xx_hal_cec.h"
#endif /* HAL_CEC_MODULE_ENABLED */

#ifdef HAL_CORTEX_MODULE_ENABLED
 #include "stm32f1xx_hal_cortex.h"
#endif /* HAL_CORTEX_MODULE_ENABLED */

#ifdef HAL_ADC_MODULE_ENABLED
 #include "stm32f1xx_hal_adc.h"
#endif /* HAL_ADC_MODULE_ENABLED */

#ifdef HAL_CRC_MODULE_ENABLED
 #include "stm32f1xx_hal_crc.h"
#endif /* HAL_CRC_MODULE_ENABLED */

#ifdef HAL_DAC_MODULE_ENABLED
 #include "stm32f1xx_hal_dac.h"
#endif /* HAL_DAC_MODULE_ENABLED */

#ifdef HAL_FLASH_MODULE_ENABLED
 #include "stm32f1xx_hal_flash.h"
#endif /* HAL_FLASH_MODULE_ENABLED */

#ifdef HAL_SRAM_MODULE_ENABLED
 #include "stm32f1xx_hal_sram.h"
#endif /* HAL_SRAM_MODULE_ENABLED */

#ifdef HAL_NOR_MODULE_ENABLED
 #include "stm32f1xx_hal_nor.h"
#endif /* HAL_NOR_MODULE_ENABLED */

#ifdef HAL_I2C_MODULE_ENABLED
 #include "stm32f1xx_hal_i2c.h"
#endif /* HAL_I2C_MODULE_ENABLED */

#ifdef HAL_I2S_MODULE_ENABLED
 #include "stm32f1xx_hal_i2s.h"
#endif /* HAL_I2S_MODULE_ENABLED */

#ifdef HAL_IWDG_MODULE_ENABLED
 #include "stm32f1xx_hal_iwdg.h"
#endif /* HAL_IWDG_MODULE_ENABLED */

#ifdef HAL_PWR_MODULE_ENABLED
 #include "stm32f1xx_hal_pwr.h"
#endif /* HAL_PWR_MODULE_ENABLED */

#ifdef HAL_RTC_MODULE_ENABLED
 #include "stm32f1xx_hal_rtc.h"
#endif /* HAL_RTC_MODULE_ENABLED */

#ifdef HAL_PCCARD_MODULE_ENABLED
 #include "stm32f1xx_hal_pccard.h"
#endif /* HAL_PCCARD_MODULE_ENABLED */ 

#ifdef HAL_SD_MODULE_ENABLED
 #include "stm32f1xx_hal_sd.h"
#endif /* HAL_SD_MODULE_ENABLED */  

#ifdef HAL_MMC_MODULE_ENABLED
 #include "stm32f1xx_hal_mmc.h"
#endif /* HAL_MMC_MODULE_ENABLED */

#ifdef HAL_NAND_MODULE_ENABLED
 #include "stm32f1xx_hal_nand.h"
#endif /* HAL_NAND_MODULE_ENABLED */     

#ifdef HAL_SPI_MODULE_ENABLED
 #include "stm32f1xx_hal_spi.h"
#endif /* HAL_SPI_MODULE_ENABLED */

#ifdef HAL_TIM_MODULE_ENABLED
 #include "stm32f1xx_hal_tim.h"
#endif /* HAL_TIM_MODULE_ENABLED */

#ifdef HAL_UART_MODULE_ENABLED
 #include "stm32f1xx_hal_uart.h"
#endif /* HAL_UART_MODULE_ENABLED */

#ifdef HAL_USART_MODULE_ENABLED
 #include "stm32f1xx_hal_usart.h"
#endif /* HAL_USART_MODULE_ENABLED */

#ifdef HAL_IRDA_MODULE_ENABLED
 #include "stm32f1xx_hal_irda.h"
#endif /* HAL_IRDA_MODULE_ENABLED */

#ifdef HAL_SMARTCARD_MODULE_ENABLED
 #include "stm32f1xx_hal_smartcard.h"
#endif /* HAL_SMARTCARD_MODULE_ENABLED */

#ifdef HAL_WWDG_MODULE_ENABLED
 #include "stm32f1xx_hal_wwdg.h"
#endif /* HAL_WWDG_MODULE_ENABLED */

#ifdef HAL_PCD_MODULE_ENABLED
 #include "stm32f1xx_hal_pcd.h"
#endif /* HAL_PCD_MODULE_ENABLED */

#ifdef HAL_HCD_MODULE_ENABLED
 #include "stm32f1xx_hal_hcd.h"
#endif /* HAL_HCD_MODULE_ENABLED */   
   

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT
/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param  expr: If expr is false, it calls assert_failed function
  *         which reports the name of the source file and the source
  *         line number of the call that failed. 
  *         If expr is true, it returns no value.
  * @retval None
  */
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/STM32F1xx_HAL/src/stm32f1xx_hal_msp.c`

```c
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : stm32f1xx_hal_msp.c
  * Description        : This file provides code for the MSP Initialization
  *                      and de-Initialization codes.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_can.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

//void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
                    /**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/

  /**NOJTAG: JTAG-DP Disabled and SW-DP Enabled
  */
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

// /**
// * @brief ADC MSP Initialization
// * This function configures the hardware resources used in this example
// * @param hadc: ADC handle pointer
// * @retval None
// */
// void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
// {

//   GPIO_InitTypeDef GPIO_InitStruct = {0};
//   if(hadc->Instance==ADC1)
//   {
//   /* USER CODE BEGIN ADC1_MspInit 0 */

//   /* USER CODE END ADC1_MspInit 0 */
//     /* Peripheral clock enable */
//     __HAL_RCC_ADC1_CLK_ENABLE();

//     __HAL_RCC_GPIOB_CLK_ENABLE();
//     /**ADC1 GPIO Configuration
//     PB1     ------> ADC1_IN9
//     */
//     GPIO_InitStruct.Pin = GPIO_PIN_1;
//     GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
//     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//   /* USER CODE BEGIN ADC1_MspInit 1 */

//   /* USER CODE END ADC1_MspInit 1 */
//   }

// }

// /**
// * @brief ADC MSP De-Initialization
// * This function freeze the hardware resources used in this example
// * @param hadc: ADC handle pointer
// * @retval None
// */

// void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
// {

//   if(hadc->Instance==ADC1)
//   {
//   /* USER CODE BEGIN ADC1_MspDeInit 0 */

//   /* USER CODE END ADC1_MspDeInit 0 */
//     /* Peripheral clock disable */
//     __HAL_RCC_ADC1_CLK_DISABLE();

//     /**ADC1 GPIO Configuration
//     PB1     ------> ADC1_IN9
//     */
//     HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);

//   /* USER CODE BEGIN ADC1_MspDeInit 1 */

//   /* USER CODE END ADC1_MspDeInit 1 */
//   }

// }

// /**
// * @brief RTC MSP Initialization
// * This function configures the hardware resources used in this example
// * @param hrtc: RTC handle pointer
// * @retval None
// */
// void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
// {

//   if(hrtc->Instance==RTC)
//   {
//   /* USER CODE BEGIN RTC_MspInit 0 */

//   /* USER CODE END RTC_MspInit 0 */
//     HAL_PWR_EnableBkUpAccess();
//     /* Enable BKP CLK enable for backup registers */
//     __HAL_RCC_BKP_CLK_ENABLE();
//     /* Peripheral clock enable */
//     __HAL_RCC_RTC_ENABLE();
//   /* USER CODE BEGIN RTC_MspInit 1 */

//   /* USER CODE END RTC_MspInit 1 */
//   }

// }

// /**
// * @brief RTC MSP De-Initialization
// * This function freeze the hardware resources used in this example
// * @param hrtc: RTC handle pointer
// * @retval None
// */

// void HAL_RTC_MspDeInit(RTC_HandleTypeDef* hrtc)
// {

//   if(hrtc->Instance==RTC)
//   {
//   /* USER CODE BEGIN RTC_MspDeInit 0 */

//   /* USER CODE END RTC_MspDeInit 0 */
//     /* Peripheral clock disable */
//     __HAL_RCC_RTC_DISABLE();
//   /* USER CODE BEGIN RTC_MspDeInit 1 */

//   /* USER CODE END RTC_MspDeInit 1 */
//   }

// }

// /**
// * @brief SPI MSP Initialization
// * This function configures the hardware resources used in this example
// * @param hspi: SPI handle pointer
// * @retval None
// */
// void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
// {

//   GPIO_InitTypeDef GPIO_InitStruct = {0};
//   if(hspi->Instance==SPI2)
//   {
//   /* USER CODE BEGIN SPI2_MspInit 0 */

//   /* USER CODE END SPI2_MspInit 0 */
//     /* Peripheral clock enable */
//     __HAL_RCC_SPI2_CLK_ENABLE();

//     __HAL_RCC_GPIOB_CLK_ENABLE();
//     /**SPI2 GPIO Configuration
//     PB13     ------> SPI2_SCK
//     PB14     ------> SPI2_MISO
//     PB15     ------> SPI2_MOSI
//     */
//     GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_15;
//     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//     GPIO_InitStruct.Pin = GPIO_PIN_14;
//     GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//     GPIO_InitStruct.Pull = GPIO_NOPULL;
//     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//   /* USER CODE BEGIN SPI2_MspInit 1 */

//   /* USER CODE END SPI2_MspInit 1 */
//   }

// }

// /**
// * @brief SPI MSP De-Initialization
// * This function freeze the hardware resources used in this example
// * @param hspi: SPI handle pointer
// * @retval None
// */

// void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
// {

//   if(hspi->Instance==SPI2)
//   {
//   /* USER CODE BEGIN SPI2_MspDeInit 0 */

//   /* USER CODE END SPI2_MspDeInit 0 */
//     /* Peripheral clock disable */
//     __HAL_RCC_SPI2_CLK_DISABLE();

//     /**SPI2 GPIO Configuration
//     PB13     ------> SPI2_SCK
//     PB14     ------> SPI2_MISO
//     PB15     ------> SPI2_MOSI
//     */
//     HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);

//   /* USER CODE BEGIN SPI2_MspDeInit 1 */

//   /* USER CODE END SPI2_MspDeInit 1 */
//   }

// }

// /**
// * @brief TIM_Base MSP Initialization
// * This function configures the hardware resources used in this example
// * @param htim_base: TIM_Base handle pointer
// * @retval None
// */
// void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
// {

//   if(htim_base->Instance==TIM2)
//   {
//   /* USER CODE BEGIN TIM2_MspInit 0 */

//   /* USER CODE END TIM2_MspInit 0 */
//     /* Peripheral clock enable */
//     __HAL_RCC_TIM2_CLK_ENABLE();
//   /* USER CODE BEGIN TIM2_MspInit 1 */

//   /* USER CODE END TIM2_MspInit 1 */
//   }
//   else if(htim_base->Instance==TIM3)
//   {
//   /* USER CODE BEGIN TIM3_MspInit 0 */

//   /* USER CODE END TIM3_MspInit 0 */
//     /* Peripheral clock enable */
//     __HAL_RCC_TIM3_CLK_ENABLE();
//   /* USER CODE BEGIN TIM3_MspInit 1 */

//   /* USER CODE END TIM3_MspInit 1 */
//   }
//   else if(htim_base->Instance==TIM4)
//   {
//   /* USER CODE BEGIN TIM4_MspInit 0 */

//   /* USER CODE END TIM4_MspInit 0 */
//     /* Peripheral clock enable */
//     __HAL_RCC_TIM4_CLK_ENABLE();
//   /* USER CODE BEGIN TIM4_MspInit 1 */

//   /* USER CODE END TIM4_MspInit 1 */
//   }

// }

// void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
// {

//   GPIO_InitTypeDef GPIO_InitStruct = {0};
//   if(htim->Instance==TIM3)
//   {
//   /* USER CODE BEGIN TIM3_MspPostInit 0 */

//   /* USER CODE END TIM3_MspPostInit 0 */

//     __HAL_RCC_GPIOC_CLK_ENABLE();
//     /**TIM3 GPIO Configuration
//     PC6     ------> TIM3_CH1
//     PC7     ------> TIM3_CH2
//     */
//     GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
//     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//     HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

//     __HAL_AFIO_REMAP_TIM3_ENABLE();

//   /* USER CODE BEGIN TIM3_MspPostInit 1 */

//   /* USER CODE END TIM3_MspPostInit 1 */
//   }

// }
// /**
// * @brief TIM_Base MSP De-Initialization
// * This function freeze the hardware resources used in this example
// * @param htim_base: TIM_Base handle pointer
// * @retval None
// */

// void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
// {

//   if(htim_base->Instance==TIM2)
//   {
//   /* USER CODE BEGIN TIM2_MspDeInit 0 */

//   /* USER CODE END TIM2_MspDeInit 0 */
//     /* Peripheral clock disable */
//     __HAL_RCC_TIM2_CLK_DISABLE();
//   /* USER CODE BEGIN TIM2_MspDeInit 1 */

//   /* USER CODE END TIM2_MspDeInit 1 */
//   }
//   else if(htim_base->Instance==TIM3)
//   {
//   /* USER CODE BEGIN TIM3_MspDeInit 0 */

//   /* USER CODE END TIM3_MspDeInit 0 */
//     /* Peripheral clock disable */
//     __HAL_RCC_TIM3_CLK_DISABLE();
//   /* USER CODE BEGIN TIM3_MspDeInit 1 */

//   /* USER CODE END TIM3_MspDeInit 1 */
//   }
//   else if(htim_base->Instance==TIM4)
//   {
//   /* USER CODE BEGIN TIM4_MspDeInit 0 */

//   /* USER CODE END TIM4_MspDeInit 0 */
//     /* Peripheral clock disable */
//     __HAL_RCC_TIM4_CLK_DISABLE();
//   /* USER CODE BEGIN TIM4_MspDeInit 1 */

//   /* USER CODE END TIM4_MspDeInit 1 */
//   }

// }

/**
* @brief UART MSP Initialization
* This function configures the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(huart->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */
  /* USER CODE END USART1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */
  /* USER CODE END USART1_MspInit 1 */
  }
  else if(huart->Instance==USART2)// yunji usart2 for console
  {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  }

}

/**
* @brief UART MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param huart: UART handle pointer
* @retval None
*/

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{

  if(huart->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */
  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */
  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(huart->Instance==USART2)
  {
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  }

}


/**
* @brief CAN MSP Initialization
* This function configures the hardware resources used in this example
* @param hcan: CAN handle pointer
* @retval None
*/
void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hcan->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN GPIO Configuration
    PA11     ------> CAN_RX
    PA12     ------> CAN_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }

}

/**
* @brief CAN MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hcan: CAN handle pointer
* @retval None
*/
void HAL_CAN_MspDeInit(CAN_HandleTypeDef* hcan)
{
  if(hcan->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN GPIO Configuration
    PA11     ------> CAN_RX
    PA12     ------> CAN_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

    /* CAN1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```

## Source: `data/rtos/xiuos/Ubiquitous/XiZi_IIoT_Macro/board/stm32f103-nano/third_party_driver/libraries/CMSIS/system_stm32f1xx.c`

```c
/**
  ******************************************************************************
  * @file    system_stm32f1xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M3 Device Peripheral Access Layer System Source File.
  * 
  * 1.  This file provides two functions and one global variable to be called from 
  *     user application:
  *      - SystemInit(): Setups the system clock (System clock source, PLL Multiplier
  *                      factors, AHB/APBx prescalers and Flash settings). 
  *                      This function is called at startup just after reset and 
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32f1xx_xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick 
  *                                  timer or configure other parameters.
  *                                     
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  * 2. After each device reset the HSI (8 MHz) is used as system clock source.
  *    Then SystemInit() function is called, in "startup_stm32f1xx_xx.s" file, to
  *    configure the system clock before to branch to main program.
  *
  * 4. The default value of HSE crystal is set to 8 MHz (or 25 MHz, depending on
  *    the product used), refer to "HSE_VALUE". 
  *    When HSE is used as system clock source, directly or through PLL, and you
  *    are using different crystal you have to adapt the HSE value to your own
  *    configuration.
  *        
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32f1xx_system
  * @{
  */  
  
/** @addtogroup STM32F1xx_System_Private_Includes
  * @{
  */

#include "stm32f1xx.h"

/**
  * @}
  */

/** @addtogroup STM32F1xx_System_Private_TypesDefinitions
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32F1xx_System_Private_Defines
  * @{
  */

#if !defined  (HSE_VALUE) 
  #define HSE_VALUE               8000000U /*!< Default value of the External oscillator in Hz.
                                                This value can be provided and adapted by the user application. */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE               8000000U /*!< Default value of the Internal oscillator in Hz.
                                                This value can be provided and adapted by the user application. */
#endif /* HSI_VALUE */

/*!< Uncomment the following line if you need to use external SRAM  */ 
#if defined(STM32F100xE) || defined(STM32F101xE) || defined(STM32F101xG) || defined(STM32F103xE) || defined(STM32F103xG)
/* #define DATA_IN_ExtSRAM */
#endif /* STM32F100xE || STM32F101xE || STM32F101xG || STM32F103xE || STM32F103xG */

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */ 
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00000000U /*!< Vector Table base offset field. 
                                  This value must be a multiple of 0x200. */


/**
  * @}
  */

/** @addtogroup STM32F1xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32F1xx_System_Private_Variables
  * @{
  */

  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency 
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
uint32_t SystemCoreClock = 16000000;
const uint8_t AHBPrescTable[16U] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8U] =  {0, 0, 0, 0, 1, 2, 3, 4};

/**
  * @}
  */

/** @addtogroup STM32F1xx_System_Private_FunctionPrototypes
  * @{
  */

#if defined(STM32F100xE) || defined(STM32F101xE) || defined(STM32F101xG) || defined(STM32F103xE) || defined(STM32F103xG)
#ifdef DATA_IN_ExtSRAM
  static void SystemInit_ExtMemCtl(void); 
#endif /* DATA_IN_ExtSRAM */
#endif /* STM32F100xE || STM32F101xE || STM32F101xG || STM32F103xE || STM32F103xG */

/**
  * @}
  */

/** @addtogroup STM32F1xx_System_Private_Functions
  * @{
  */

/**
  * @brief  Setup the microcontroller system
  *         Initialize the Embedded Flash Interface, the PLL and update the 
  *         SystemCoreClock variable.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
void SystemInit (void)
{
  /* Reset the RCC clock configuration to the default reset state(for debug purpose) */
  /* Set HSION bit */
  RCC->CR |= 0x00000001U;

  /* Reset SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits */
#if !defined(STM32F105xC) && !defined(STM32F107xC)
  RCC->CFGR &= 0xF8FF0000U;
#else
  RCC->CFGR &= 0xF0FF0000U;
#endif /* STM32F105xC */   
  
  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= 0xFEF6FFFFU;

  /* Reset HSEBYP bit */
  RCC->CR &= 0xFFFBFFFFU;

  /* Reset PLLSRC, PLLXTPRE, PLLMUL and USBPRE/OTGFSPRE bits */
  RCC->CFGR &= 0xFF80FFFFU;

#if defined(STM32F105xC) || defined(STM32F107xC)
  /* Reset PLL2ON and PLL3ON bits */
  RCC->CR &= 0xEBFFFFFFU;

  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x00FF0000U;

  /* Reset CFGR2 register */
  RCC->CFGR2 = 0x00000000U;
#elif defined(STM32F100xB) || defined(STM32F100xE)
  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x009F0000U;

  /* Reset CFGR2 register */
  RCC->CFGR2 = 0x00000000U;      
#else
  /* Disable all interrupts and clear pending bits  */
  RCC->CIR = 0x009F0000U;
#endif /* STM32F105xC */
    
#if defined(STM32F100xE) || defined(STM32F101xE) || defined(STM32F101xG) || defined(STM32F103xE) || defined(STM32F103xG)
  #ifdef DATA_IN_ExtSRAM
    SystemInit_ExtMemCtl(); 
  #endif /* DATA_IN_ExtSRAM */
#endif 

#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM. */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH. */
#endif 
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *           
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.         
  *     
  * @note   - The system frequency computed by this function is not the real 
  *           frequency in the chip. It is calculated based on the predefined 
  *           constant and the selected clock source:
  *             
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(*)
  *                                              
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(**)
  *                          
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(**) 
  *             or HSI_VALUE(*) multiplied by the PLL factors.
  *         
  *         (*) HSI_VALUE is a constant defined in stm32f1xx.h file (default value
  *             8 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.   
  *    
  *         (**) HSE_VALUE is a constant defined in stm32f1xx.h file (default value
  *              8 MHz or 25 MHz, depending on the product used), user has to ensure
  *              that HSE_VALUE is same as the real frequency of the crystal used.
  *              Otherwise, this function may have wrong result.
  *                
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate (void)
{
  uint32_t tmp = 0U, pllmull = 0U, pllsource = 0U;

#if defined(STM32F105xC) || defined(STM32F107xC)
  uint32_t prediv1source = 0U, prediv1factor = 0U, prediv2factor = 0U, pll2mull = 0U;
#endif /* STM32F105xC */

#if defined(STM32F100xB) || defined(STM32F100xE)
  uint32_t prediv1factor = 0U;
#endif /* STM32F100xB or STM32F100xE */
    
  /* Get SYSCLK source -------------------------------------------------------*/
  tmp = RCC->CFGR & RCC_CFGR_SWS;
  
  switch (tmp)
  {
    case 0x00U:  /* HSI used as system clock */
      SystemCoreClock = HSI_VALUE;
      break;
    case 0x04U:  /* HSE used as system clock */
      SystemCoreClock = HSE_VALUE;
      break;
    case 0x08U:  /* PLL used as system clock */

      /* Get PLL clock source and multiplication factor ----------------------*/
      pllmull = RCC->CFGR & RCC_CFGR_PLLMULL;
      pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;
      
#if !defined(STM32F105xC) && !defined(STM32F107xC)      
      pllmull = ( pllmull >> 18U) + 2U;
      
      if (pllsource == 0x00U)
      {
        /* HSI oscillator clock divided by 2 selected as PLL clock entry */
        SystemCoreClock = (HSI_VALUE >> 1U) * pllmull;
      }
      else
      {
 #if defined(STM32F100xB) || defined(STM32F100xE)
       prediv1factor = (RCC->CFGR2 & RCC_CFGR2_PREDIV1) + 1U;
       /* HSE oscillator clock selected as PREDIV1 clock entry */
       SystemCoreClock = (HSE_VALUE / prediv1factor) * pllmull; 
 #else
        /* HSE selected as PLL clock entry */
        if ((RCC->CFGR & RCC_CFGR_PLLXTPRE) != (uint32_t)RESET)
        {/* HSE oscillator clock divided by 2 */
          SystemCoreClock = (HSE_VALUE >> 1U) * pllmull;
        }
        else
        {
          SystemCoreClock = HSE_VALUE * pllmull;
        }
 #endif
      }
#else
      pllmull = pllmull >> 18U;
      
      if (pllmull != 0x0DU)
      {
         pllmull += 2U;
      }
      else
      { /* PLL multiplication factor = PLL input clock * 6.5 */
        pllmull = 13U / 2U; 
      }
            
      if (pllsource == 0x00U)
      {
        /* HSI oscillator clock divided by 2 selected as PLL clock entry */
        SystemCoreClock = (HSI_VALUE >> 1U) * pllmull;
      }
      else
      {/* PREDIV1 selected as PLL clock entry */
        
        /* Get PREDIV1 clock source and division factor */
        prediv1source = RCC->CFGR2 & RCC_CFGR2_PREDIV1SRC;
        prediv1factor = (RCC->CFGR2 & RCC_CFGR2_PREDIV1) + 1U;
        
        if (prediv1source == 0U)
        { 
          /* HSE oscillator clock selected as PREDIV1 clock entry */
          SystemCoreClock = (HSE_VALUE / prediv1factor) * pllmull;          
        }
        else
        {/* PLL2 clock selected as PREDIV1 clock entry */
          
          /* Get PREDIV2 division factor and PLL2 multiplication factor */
          prediv2factor = ((RCC->CFGR2 & RCC_CFGR2_PREDIV2) >> 4U) + 1U;
          pll2mull = ((RCC->CFGR2 & RCC_CFGR2_PLL2MUL) >> 8U) + 2U; 
          SystemCoreClock = (((HSE_VALUE / prediv2factor) * pll2mull) / prediv1factor) * pllmull;                         
        }
      }
#endif /* STM32F105xC */ 
      break;

    default:
      SystemCoreClock = HSI_VALUE;
      break;
  }
  
  /* Compute HCLK clock frequency ----------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4U)];
  /* HCLK clock frequency */
  SystemCoreClock >>= tmp;  
}

#if defined(STM32F100xE) || defined(STM32F101xE) || defined(STM32F101xG) || defined(STM32F103xE) || defined(STM32F103xG)
/**
  * @brief  Setup the external memory controller. Called in startup_stm32f1xx.s 
  *          before jump to __main
  * @param  None
  * @retval None
  */ 
#ifdef DATA_IN_ExtSRAM
/**
  * @brief  Setup the external memory controller. 
  *         Called in startup_stm32f1xx_xx.s/.c before jump to main.
  *         This function configures the external SRAM mounted on STM3210E-EVAL
  *         board (STM32 High density devices). This SRAM will be used as program
  *         data memory (including heap and stack).
  * @param  None
  * @retval None
  */ 
void SystemInit_ExtMemCtl(void) 
{
  __IO uint32_t tmpreg;
  /*!< FSMC Bank1 NOR/SRAM3 is used for the STM3210E-EVAL, if another Bank is 
    required, then adjust the Register Addresses */

  /* Enable FSMC clock */
  RCC->AHBENR = 0x00000114U;

  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->AHBENR, RCC_AHBENR_FSMCEN);
  
  /* Enable GPIOD, GPIOE, GPIOF and GPIOG clocks */
  RCC->APB2ENR = 0x000001E0U;
  
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPDEN);

  (void)(tmpreg);
  
/* ---------------  SRAM Data lines, NOE and NWE configuration ---------------*/
/*----------------  SRAM Address lines configuration -------------------------*/
/*----------------  NOE and NWE configuration --------------------------------*/  
/*----------------  NE3 configuration ----------------------------------------*/
/*----------------  NBL0, NBL1 configuration ---------------------------------*/
  
  GPIOD->CRL = 0x44BB44BBU;  
  GPIOD->CRH = 0xBBBBBBBBU;

  GPIOE->CRL = 0xB44444BBU;  
  GPIOE->CRH = 0xBBBBBBBBU;

  GPIOF->CRL = 0x44BBBBBBU;  
  GPIOF->CRH = 0xBBBB4444U;

  GPIOG->CRL = 0x44BBBBBBU;  
  GPIOG->CRH = 0x444B4B44U;
   
/*----------------  FSMC Configuration ---------------------------------------*/  
/*----------------  Enable FSMC Bank1_SRAM Bank ------------------------------*/
  
  FSMC_Bank1->BTCR[4U] = 0x00001091U;
  FSMC_Bank1->BTCR[5U] = 0x00110212U;
}
#endif /* DATA_IN_ExtSRAM */
#endif /* STM32F100xE || STM32F101xE || STM32F101xG || STM32F103xE || STM32F103xG */

/**
  * @}
  */

/**
  * @}
  */
  
/**
  * @}
  */    
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
```
