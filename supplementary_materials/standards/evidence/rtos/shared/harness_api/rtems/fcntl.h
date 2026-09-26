#ifndef DRIVERGEN_RTEMS_FCNTL_H
#define DRIVERGEN_RTEMS_FCNTL_H

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002

int open(const char *path, int oflag, ...);

#endif
