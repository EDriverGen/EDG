/*
 * DS18B20 sensor driver for ChibiOS
 */
#include "ds18b20_ref.h"

static const uint8_t crc8_table[256] = {
    0,94,188,226,97,63,221,131,194,156,126,32,163,253,31,65,
    157,195,33,127,252,162,64,30,95,1,227,189,62,96,130,220,
    35,125,159,193,66,28,254,160,225,191,93,3,128,222,60,98,
    190,224,2,92,223,129,99,61,124,34,192,158,29,67,161,255,
    70,24,250,164,39,121,155,197,132,218,56,102,229,187,89,7,
    219,133,103,57,186,228,6,88,25,71,165,251,120,38,196,154,
    101,59,217,135,4,90,184,230,167,249,27,69,198,152,122,36,
    248,166,68,26,153,199,37,123,58,100,134,216,91,5,231,185,
    140,210,48,110,237,179,81,15,78,16,242,172,47,113,147,205,
    17,79,173,243,112,46,204,146,211,141,111,49,178,236,14,80,
    175,241,19,77,206,144,114,44,109,51,209,143,12,82,176,238,
    50,108,142,208,83,13,239,177,240,174,76,18,145,207,45,115,
    202,148,118,40,171,245,23,73,8,86,180,234,105,55,213,139,
    87,9,235,181,54,104,138,212,149,203,41,119,244,170,72,22,
    233,183,85,11,136,214,52,106,43,117,151,201,74,20,246,168,
    116,42,200,150,21,75,169,247,182,232,10,84,215,137,107,53
};

static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
        crc = crc8_table[crc ^ data[i]];
    return crc;
}

#define OW_DELAY(us) chSysPolledDelayX(US2RTC(STM32_HCLK,(us)))

static int ow_reset(ioportid_t port, ioportmask_t pad)
{
    int present;
    palSetPadMode(port,pad,PAL_MODE_OUTPUT_OPENDRAIN); palClearPad(port,pad);
    OW_DELAY(480);
    palSetPadMode(port,pad,PAL_MODE_INPUT_PULLUP); OW_DELAY(60);
    present=(palReadPad(port,pad)==PAL_LOW)?0:-1;
    OW_DELAY(420); return present;
}

static void ow_write_bit(ioportid_t port, ioportmask_t pad, int bit)
{ palSetPadMode(port,pad,PAL_MODE_OUTPUT_OPENDRAIN); palClearPad(port,pad);
  if(bit){OW_DELAY(5);palSetPad(port,pad);OW_DELAY(55);}
  else{OW_DELAY(60);palSetPad(port,pad);OW_DELAY(5);}
}

static int ow_read_bit(ioportid_t port, ioportmask_t pad)
{ int v; palSetPadMode(port,pad,PAL_MODE_OUTPUT_OPENDRAIN); palClearPad(port,pad);
  OW_DELAY(2); palSetPadMode(port,pad,PAL_MODE_INPUT_PULLUP); OW_DELAY(12);
  v=(palReadPad(port,pad)==PAL_HIGH)?1:0; OW_DELAY(50); return v;
}

static void ow_write_byte(ioportid_t port, ioportmask_t pad, uint8_t b)
{ for(int i=0;i<8;i++){ow_write_bit(port,pad,b&1);b>>=1;} }

static uint8_t ow_read_byte(ioportid_t port, ioportmask_t pad)
{ uint8_t v=0; for(int i=0;i<8;i++) v|=(ow_read_bit(port,pad)<<i); return v; }

int ds18b20_init(struct ds18b20_device *dev, ioportid_t port, ioportmask_t pad)
{ if(!dev)return -1; dev->port=port; dev->pad=pad; return ow_reset(port,pad); }

int ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100)
{
    uint8_t buf[9];
    if(!dev||!temp_x100)return -1;
    if(ow_reset(dev->port,dev->pad)!=0)return -1;
    ow_write_byte(dev->port,dev->pad, DS18B20_CMD_SKIP_ROM);
    ow_write_byte(dev->port,dev->pad, DS18B20_CMD_CONVERT_T);
    chThdSleepMilliseconds(DS18B20_CONVERT_WAIT_MS);
    if(ow_reset(dev->port,dev->pad)!=0)return -1;
    ow_write_byte(dev->port,dev->pad, DS18B20_CMD_SKIP_ROM);
    ow_write_byte(dev->port,dev->pad, DS18B20_CMD_READ_SCRATCH);
    for(int i=0;i<9;i++) buf[i]=ow_read_byte(dev->port,dev->pad);

    /* verify CRC */
    if (ds18b20_crc8(buf, 8) != buf[8]) return -1;
    int16_t raw = (int16_t)((uint16_t)buf[1] << 8 | buf[0]);
    *temp_x100 = (int32_t)raw * 100 / 16;  /* 0.0625 degC per LSB */
    return 0;
}
