/* OpenHarmony HDF uart_if.h stub */
#ifndef __UART_IF_STUB_H
#define __UART_IF_STUB_H

#include "openharmony_liteosm.h"

/* UART configuration */
struct UartAttribute {
    uint32_t dataBits;
    uint32_t parity;
    uint32_t stopBits;
    uint32_t rts;
    uint32_t cts;
    uint32_t fifoRxEn;
    uint32_t fifoTxEn;
};

#define UART_ATTR_DATABIT_8    0
#define UART_ATTR_DATABIT_7    1
#define UART_ATTR_PARITY_NONE  0
#define UART_ATTR_PARITY_ODD   1
#define UART_ATTR_PARITY_EVEN  2
#define UART_ATTR_STOPBIT_1    0
#define UART_ATTR_STOPBIT_2    1
#define UART_ATTR_RTS_DIS      0
#define UART_ATTR_CTS_DIS      0

DevHandle UartOpen(uint32_t port);
void UartClose(DevHandle handle);
int32_t UartRead(DevHandle handle, uint8_t *data, uint32_t size);
int32_t UartWrite(DevHandle handle, uint8_t *data, uint32_t size);
int32_t UartSetBaud(DevHandle handle, uint32_t baudRate);
int32_t UartGetBaud(DevHandle handle, uint32_t *baudRate);
int32_t UartSetAttribute(DevHandle handle, struct UartAttribute *attribute);
int32_t UartGetAttribute(DevHandle handle, struct UartAttribute *attribute);
int32_t UartSetTransMode(DevHandle handle, uint32_t mode);

#endif /* __UART_IF_STUB_H */
