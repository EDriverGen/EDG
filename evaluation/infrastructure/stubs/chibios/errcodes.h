/*
 * ChibiOS errcodes.h sandbox stub for DriverGen.
 *
 * Real upstream: data/rtos/chibios/os/common/utils/include/errcodes.h
 * verified 2026-04-27
 *
 * forwarder strategy:
 *   1. include chibios.h to get MSG_OK / MSG_TIMEOUT / msg_t (referenced
 *      by CH_RET_SUCCESS / CH_RET_TIMEOUT below).
 *   2. supply CH_RET_* macros + CH_ENCODE_ERROR / CH_DECODE_ERROR /
 *      CH_RET_IS_* exactly as the real upstream defines them, so drivers
 *      that use the user-facing error code namespace compile cleanly.
 *
 * NOTE: macros below mirror upstream byte-for-byte; if upstream changes,
 *       re-verify and bump the date above.
 */
#ifndef ERRCODES_H
#define ERRCODES_H

#include "chibios.h"
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CH_ERRORS_MASK              (int)0xFF
#define CH_ENCODE_ERROR(posixerr)   (~CH_ERRORS_MASK | (int)(posixerr))
#define CH_DECODE_ERROR(err)        ((int)(err) & CH_ERRORS_MASK)
#define CH_RET_IS_ERROR(x)          (((int)(x) & ~CH_ERRORS_MASK) == ~CH_ERRORS_MASK)
#define CH_RET_IS_SUCCESS(x)        (((int)(x) & ~CH_ERRORS_MASK) != ~CH_ERRORS_MASK)

#define CH_RET_SUCCESS              (int)MSG_OK
#define CH_RET_TIMEOUT              (int)MSG_TIMEOUT
#define CH_RET_INNER_ERROR          (int)-3

#define CH_RET_ENOENT               CH_ENCODE_ERROR(ENOENT)
#define CH_RET_EIO                  CH_ENCODE_ERROR(EIO)
#define CH_RET_EBADF                CH_ENCODE_ERROR(EBADF)
#define CH_RET_ENOMEM               CH_ENCODE_ERROR(ENOMEM)
#define CH_RET_EACCES               CH_ENCODE_ERROR(EACCES)
#define CH_RET_EFAULT               CH_ENCODE_ERROR(EFAULT)
#define CH_RET_EBUSY                CH_ENCODE_ERROR(EBUSY)
#define CH_RET_EEXIST               CH_ENCODE_ERROR(EEXIST)
#define CH_RET_ENOTDIR              CH_ENCODE_ERROR(ENOTDIR)
#define CH_RET_EISDIR               CH_ENCODE_ERROR(EISDIR)
#define CH_RET_EINVAL               CH_ENCODE_ERROR(EINVAL)
#define CH_RET_EMFILE               CH_ENCODE_ERROR(EMFILE)
#define CH_RET_ENFILE               CH_ENCODE_ERROR(ENFILE)
#define CH_RET_EFBIG                CH_ENCODE_ERROR(EFBIG)
#define CH_RET_ENOSPC               CH_ENCODE_ERROR(ENOSPC)
#define CH_RET_ESPIPE               CH_ENCODE_ERROR(ESPIPE)
#define CH_RET_EROFS                CH_ENCODE_ERROR(EROFS)
#define CH_RET_ERANGE               CH_ENCODE_ERROR(ERANGE)
#define CH_RET_ENAMETOOLONG         CH_ENCODE_ERROR(ENAMETOOLONG)
#define CH_RET_ENOSYS               CH_ENCODE_ERROR(ENOSYS)
#define CH_RET_EOVERFLOW            CH_ENCODE_ERROR(EOVERFLOW)
#define CH_RET_ENOEXEC              CH_ENCODE_ERROR(ENOEXEC)
#define CH_RET_EXDEV                CH_ENCODE_ERROR(EXDEV)

#define CH_BREAK_ON_ERROR(err)                                              \
  if (CH_RET_IS_ERROR(err)) break

#define CH_BREAK_ON_SUCCESS(err)                                            \
  if (CH_RET_IS_SUCCESS(err)) break

#define CH_RETURN_ON_ERROR(err) do {                                        \
  int __ret = (err);                                                        \
  if (CH_RET_IS_ERROR(__ret)) {                                             \
    return __ret;                                                           \
  }                                                                         \
} while (false)

#define CH_RETURN_ON_SUCCESS(err) do {                                      \
  int __ret = (err);                                                        \
  if (CH_RET_IS_SUCCESS(__ret)) {                                           \
    return __ret;                                                           \
  }                                                                         \
} while (false)

#ifdef __cplusplus
}
#endif

#endif /* ERRCODES_H */
