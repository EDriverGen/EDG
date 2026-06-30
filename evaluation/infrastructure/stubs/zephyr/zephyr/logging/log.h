#ifndef DRIVERGEN_ZEPHYR_LOGGING_LOG_H
#define DRIVERGEN_ZEPHYR_LOGGING_LOG_H

#ifndef LOG_MODULE_REGISTER
#define LOG_MODULE_REGISTER(...)
#endif
#ifndef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)
#endif

#ifndef LOG_ERR
#define LOG_ERR(...) do { } while (0)
#endif
#ifndef LOG_WRN
#define LOG_WRN(...) do { } while (0)
#endif
#ifndef LOG_INF
#define LOG_INF(...) do { } while (0)
#endif
#ifndef LOG_DBG
#define LOG_DBG(...) do { } while (0)
#endif

#ifndef LOG_HEXDUMP_ERR
#define LOG_HEXDUMP_ERR(...) do { } while (0)
#endif
#ifndef LOG_HEXDUMP_WRN
#define LOG_HEXDUMP_WRN(...) do { } while (0)
#endif
#ifndef LOG_HEXDUMP_INF
#define LOG_HEXDUMP_INF(...) do { } while (0)
#endif
#ifndef LOG_HEXDUMP_DBG
#define LOG_HEXDUMP_DBG(...) do { } while (0)
#endif

#endif /* DRIVERGEN_ZEPHYR_LOGGING_LOG_H */
