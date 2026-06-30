/* NuttX stub implementations */
#include "nuttx.h"
#include <stdlib.h>

/* File I/O stubs */
int open(const char *path, int oflag, ...) { (void)path; (void)oflag; return 3; }
int close(int fd) { (void)fd; return 0; }
int read(int fd, void *buf, size_t n) { (void)fd; if(buf&&n) memset(buf,0x19,n); return (int)n; }
int write(int fd, const void *buf, size_t n) { (void)fd; (void)buf; return (int)n; }
int ioctl(int fd, int req, ...) { (void)fd; (void)req; return 0; }

/* Timing */
int usleep(unsigned int usec) { (void)usec; return 0; }
unsigned int sleep(unsigned int sec) { (void)sec; return 0; }
int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    (void)clk_id; if(tp){ tp->tv_sec=0; tp->tv_nsec=0; } return 0;
}
void up_mdelay(unsigned int ms) { (void)ms; }
void up_udelay(unsigned int us) { (void)us; }

/* I2C */
int I2C_TRANSFER(struct i2c_master_s *dev, struct i2c_msg_s *msgs, int count) {
    (void)dev; (void)msgs; (void)count; return OK;
}
int i2c_write(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
              FAR const uint8_t *buffer, int buflen) {
    (void)dev; (void)config; (void)buffer; (void)buflen; return OK;
}
int i2c_read(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
             FAR uint8_t *buffer, int buflen) {
    (void)dev; (void)config;
    if(buffer && buflen>0) memset(buffer,0x19,buflen); return OK;
}
int i2c_writeread(FAR struct i2c_master_s *dev, FAR const struct i2c_config_s *config,
                  FAR const uint8_t *wbuf, int wlen,
                  FAR uint8_t *rbuf, int rlen) {
    (void)dev; (void)config; (void)wbuf; (void)wlen;
    if(rbuf && rlen>0) memset(rbuf,0x19,rlen); return OK;
}
struct i2c_master_s *board_i2cbus_initialize(int bus) { (void)bus; static struct i2c_master_s inst; return &inst; }
int board_i2cbus_uninitialize(struct i2c_master_s *dev) { (void)dev; return OK; }

/* printf/syslog */
int printf(const char *fmt, ...) { (void)fmt; return 0; }
int snprintf(char *buf, size_t size, const char *fmt, ...) { (void)buf; (void)size; (void)fmt; return 0; }
int syslog(int priority, const char *fmt, ...) { (void)priority; (void)fmt; return 0; }

/* Memory */
void *kmm_malloc(size_t size) { return malloc(size); }
void *kmm_zalloc(size_t size) { return calloc(1, size); }
void  kmm_free(void *ptr) { free(ptr); }

__attribute__((weak)) int main(void) { return 0; }
