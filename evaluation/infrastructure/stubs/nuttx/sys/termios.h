/* NuttX sys/termios.h stub */
#ifndef __NUTTX_SYS_TERMIOS_STUB_H
#define __NUTTX_SYS_TERMIOS_STUB_H

#include <stdint.h>

typedef unsigned int tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int speed_t;

#define NCCS 20

/* c_cc indices */
#define VMIN    6
#define VTIME   5
#define VEOF    4
#define VEOL    11
#define VERASE  2
#define VKILL   3
#define VINTR   0
#define VQUIT   1
#define VSTART  8
#define VSTOP   9

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t     c_cc[NCCS];
};

/* c_cflag bit masks */
#define CSIZE   0x00000030
#define CS5     0x00000000
#define CS6     0x00000010
#define CS7     0x00000020
#define CS8     0x00000030
#define CSTOPB  0x00000040
#define CREAD   0x00000080
#define PARENB  0x00000100
#define PARODD  0x00000200
#define CLOCAL  0x00000800
#define CRTSCTS 0x80000000

/* Baud rate */
#define B9600   9600
#define B19200  19200
#define B38400  38400
#define B57600  57600
#define B115200 115200

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);

#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#endif
