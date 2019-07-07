#ifndef __C99_ENDIAN__
#define __C99_ENDIAN__

/* this functionality is copied from int-util.h but made compatible with C99 cross-platform */
/* you need to use these macros with clang: (\__BIG_ENDIAN__ or \__LITTLE_ENDIAN__) */

#include <memory.h>
#include <stdio.h>
#include <sys/param.h>

static inline uint32_t rol32(uint32_t x, int r) { return (x << (r & 31)) | (x >> (-r & 31)); }
static inline uint64_t rol64(uint64_t x, int r) { return (x << (r & 63)) | (x >> (-r & 63)); }

#define IDENT32(x) ((uint32_t) (x))
#define IDENT64(x) ((uint64_t) (x))

#define SWAP32(x) ((((uint32_t) (x) & 0x000000ff) << 24) | \
  (((uint32_t) (x) & 0x0000ff00) <<  8) | \
  (((uint32_t) (x) & 0x00ff0000) >>  8) | \
  (((uint32_t) (x) & 0xff000000) >> 24))
#define SWAP64(x) ((((uint64_t) (x) & 0x00000000000000ff) << 56) | \
  (((uint64_t) (x) & 0x000000000000ff00) << 40) | \
  (((uint64_t) (x) & 0x0000000000ff0000) << 24) | \
  (((uint64_t) (x) & 0x00000000ff000000) <<  8) | \
  (((uint64_t) (x) & 0x000000ff00000000) >>  8) | \
  (((uint64_t) (x) & 0x0000ff0000000000) >> 24) | \
  (((uint64_t) (x) & 0x00ff000000000000) >> 40) | \
  (((uint64_t) (x) & 0xff00000000000000) >> 56))

static inline uint32_t ident32(uint32_t x) { return x; }
static inline uint64_t ident64(uint64_t x) { return x; }

static inline uint32_t swap32(uint32_t x) {
  x = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);
  return (x << 16) | (x >> 16);
}
static inline uint64_t swap64(uint64_t x) {
  x = ((x & 0x00ff00ff00ff00ff) <<  8) | ((x & 0xff00ff00ff00ff00) >>  8);
  x = ((x & 0x0000ffff0000ffff) << 16) | ((x & 0xffff0000ffff0000) >> 16);
  return (x << 32) | (x >> 32);
}

#define BIG_ENDIAN      4321 /* byte 0 is most significant (mc68k) */
#define LITTLE_ENDIAN   1234 /* byte 0 is least significant (i386) */

#if __LITTLE_ENDIAN__
  #define BYTE_ORDER LITTLE_ENDIAN

  #define SWAP32LE IDENT32
  #define SWAP32BE SWAP32
  #define swap32le ident32
  #define swap32be swap32
  #define mem_inplace_swap32le mem_inplace_ident
  #define mem_inplace_swap32be mem_inplace_swap32
  #define memcpy_swap32le memcpy_ident32
  #define memcpy_swap32be memcpy_swap32
  #define SWAP64LE IDENT64
  #define SWAP64BE SWAP64
  #define swap64le ident64
  #define swap64be swap64
  #define mem_inplace_swap64le mem_inplace_ident
  #define mem_inplace_swap64be mem_inplace_swap64
  #define memcpy_swap64le memcpy_ident64
  #define memcpy_swap64be memcpy_swap64
#else
  #define BYTE_ORDER BIG_ENDIAN

  #define SWAP32BE IDENT32
  #define SWAP32LE SWAP32
  #define swap32be ident32
  #define swap32le swap32
  #define mem_inplace_swap32be mem_inplace_ident
  #define mem_inplace_swap32le mem_inplace_swap32
  #define memcpy_swap32be memcpy_ident32
  #define memcpy_swap32le memcpy_swap32
  #define SWAP64BE IDENT64
  #define SWAP64LE SWAP64
  #define swap64be ident64
  #define swap64le swap64
  #define mem_inplace_swap64be mem_inplace_ident
  #define mem_inplace_swap64le mem_inplace_swap64
  #define memcpy_swap64be memcpy_ident64
  #define memcpy_swap64le memcpy_swap64
#endif

#endif // __C99_ENDIAN__
