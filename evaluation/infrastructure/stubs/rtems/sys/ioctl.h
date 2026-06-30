#ifndef DRIVERGEN_RTEMS_SYS_IOCTL_H
#define DRIVERGEN_RTEMS_SYS_IOCTL_H

#include <stdint.h>

typedef unsigned long ioctl_command_t;

#ifndef _IO
#define _IO(type, nr) ((unsigned long)(((type) << 8) | (nr)))
#endif
#ifndef _IOW
#define _IOW(type, nr, data) _IO(type, nr)
#endif
#ifndef _IOR
#define _IOR(type, nr, data) _IO(type, nr)
#endif

int ioctl(int fd, unsigned long request, ...);

#endif
