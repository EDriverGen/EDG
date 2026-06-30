/*
 * Stub for <zephyr/sys/byteorder.h> in DriverGen's Zephyr stub environment.
 *
 * Upstream Zephyr ships sys_{be,le,cpu}_to_{cpu,be,le}_{16,24,32,48,64}
 * helpers as ordinary inline functions / macros. We reproduce just enough
 * of that surface for driver compilation.  Semantics assume the target
 * MCU is little-endian (true for every Cortex-M we emulate in Renode);
 * swap helpers use the standard GCC/Clang builtins available in the
 * arm-none-eabi toolchain.
 */
#ifndef ZEPHYR_SYS_BYTEORDER_H_
#define ZEPHYR_SYS_BYTEORDER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Byte swap primitives --------------------------------------------------- */
#ifndef __bswap_16
static inline uint16_t __bswap_16(uint16_t v) {
    return (uint16_t)((v << 8) | (v >> 8));
}
#endif

#ifndef __bswap_32
static inline uint32_t __bswap_32(uint32_t v) {
    return ((v & 0x000000FFu) << 24) |
           ((v & 0x0000FF00u) <<  8) |
           ((v & 0x00FF0000u) >>  8) |
           ((v & 0xFF000000u) >> 24);
}
#endif

#ifndef __bswap_64
static inline uint64_t __bswap_64(uint64_t v) {
    return ((v & 0x00000000000000FFull) << 56) |
           ((v & 0x000000000000FF00ull) << 40) |
           ((v & 0x0000000000FF0000ull) << 24) |
           ((v & 0x00000000FF000000ull) <<  8) |
           ((v & 0x000000FF00000000ull) >>  8) |
           ((v & 0x0000FF0000000000ull) >> 24) |
           ((v & 0x00FF000000000000ull) >> 40) |
           ((v & 0xFF00000000000000ull) >> 56);
}
#endif

/* Endianness helpers (target assumed little-endian) ---------------------- */
#define sys_cpu_to_le16(val) ((uint16_t)(val))
#define sys_cpu_to_le32(val) ((uint32_t)(val))
#define sys_cpu_to_le64(val) ((uint64_t)(val))
#define sys_le16_to_cpu(val) ((uint16_t)(val))
#define sys_le32_to_cpu(val) ((uint32_t)(val))
#define sys_le64_to_cpu(val) ((uint64_t)(val))

#define sys_cpu_to_be16(val) __bswap_16((uint16_t)(val))
#define sys_cpu_to_be32(val) __bswap_32((uint32_t)(val))
#define sys_cpu_to_be64(val) __bswap_64((uint64_t)(val))
#define sys_be16_to_cpu(val) __bswap_16((uint16_t)(val))
#define sys_be32_to_cpu(val) __bswap_32((uint32_t)(val))
#define sys_be64_to_cpu(val) __bswap_64((uint64_t)(val))

/* 24-bit helpers: rarely used but present in upstream zephyr --------- */
static inline uint32_t sys_be24_to_cpu(uint32_t v) {
    return ((v & 0x00FF0000u) >> 16) |
           ((v & 0x0000FF00u)      ) |
           ((v & 0x000000FFu) << 16);
}
static inline uint32_t sys_cpu_to_be24(uint32_t v) { return sys_be24_to_cpu(v); }
static inline uint32_t sys_le24_to_cpu(uint32_t v) { return v & 0x00FFFFFFu; }
static inline uint32_t sys_cpu_to_le24(uint32_t v) { return v & 0x00FFFFFFu; }

/* Buffer-style helpers for streaming I/O ----------------------------- */
static inline void sys_put_be16(uint16_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val >> 8); dst[1] = (uint8_t)(val & 0xFFu);
}
static inline void sys_put_le16(uint16_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val & 0xFFu); dst[1] = (uint8_t)(val >> 8);
}
static inline void sys_put_be32(uint32_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val >> 24);
    dst[1] = (uint8_t)(val >> 16);
    dst[2] = (uint8_t)(val >> 8);
    dst[3] = (uint8_t)(val & 0xFFu);
}
static inline void sys_put_le32(uint32_t val, uint8_t *dst) {
    dst[0] = (uint8_t)(val & 0xFFu);
    dst[1] = (uint8_t)(val >> 8);
    dst[2] = (uint8_t)(val >> 16);
    dst[3] = (uint8_t)(val >> 24);
}
static inline uint16_t sys_get_be16(const uint8_t *src) {
    return (uint16_t)((src[0] << 8) | src[1]);
}
static inline uint16_t sys_get_le16(const uint8_t *src) {
    return (uint16_t)(src[0] | (src[1] << 8));
}
static inline uint32_t sys_get_be32(const uint8_t *src) {
    return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
           ((uint32_t)src[2] <<  8) |  (uint32_t)src[3];
}
static inline uint32_t sys_get_le32(const uint8_t *src) {
    return  (uint32_t)src[0]        | ((uint32_t)src[1] <<  8) |
           ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SYS_BYTEORDER_H_ */
