/* Verified against RTEMS POSIX unistd surface and I2C users on 2026-05-12. */
#ifndef DRIVERGEN_RTEMS_UNISTD_H
#define DRIVERGEN_RTEMS_UNISTD_H

#include <stddef.h>
#include <sys/types.h>

typedef unsigned long useconds_t;

int close(int fd);
ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t write(int fd, const void *buf, size_t nbyte);
int usleep(useconds_t usec);

#endif
