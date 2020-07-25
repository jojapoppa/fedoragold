// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if !defined(__arm__)
#include <emmintrin.h>
#include <wmmintrin.h>
#else
#include <arm_neon.h>
typedef uint8x16_t __m128i;
inline __m128i _mm_shuffle_epi32 (__m128i a, int imm)
{
  switch (imm) {
      case 0 :
        return vdupq_n_s32(vgetq_lane_s32(a, 0)); 
        break;
      default: 
        //__m128i ret;
        uint8x16_t ret;
        ret[0] = a[imm & 0x3];
        ret[1] = a[(imm >> 2) & 0x3];
        ret[2] = a[(imm >> 4) & 0x03];
        ret[3] = a[(imm >> 6) & 0x03];
        return ret;
  }
}
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#else

#if !defined (__arm__)
#include <cpuid.h>
#endif

#endif

#include "crypto/aesb.h"
#include "crypto/initializer.h"
//#include "Common/int-util.h"
#include "crypto/hash-ops.h"
#include "crypto/oaes_lib.h"

void (*cn_slow_hash_fp)(size_t, void *, const void *, size_t, void *, bool);

void cn_slow_hash_f(size_t v, void * a, const void * b, size_t c, void * d, bool e){
(*cn_slow_hash_fp)(v, a, b, c, d, e);
}

#if defined(__GNUC__)
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define __attribute__(x)
#endif

#if defined(_MSC_VER)
#define restrict
#endif

#define MEMORY         (1 << 21) /* 2 MiB */
#define ITER           (1 << 20)
#define AES_BLOCK_SIZE  16
#define AES_KEY_SIZE    32 /*16*/
#define INIT_SIZE_BLK   8
#define INIT_SIZE_BYTE (INIT_SIZE_BLK * AES_BLOCK_SIZE)	// 128

#pragma pack(push, 1)
union cn_slow_hash_state {
  union hash_state hs;
  struct {
    uint8_t k[64];
    uint8_t init[INIT_SIZE_BYTE];
  };
};
#pragma pack(pop)

#if defined(_MSC_VER)
#define ALIGNED_DATA(x) __declspec(align(x))
#define ALIGNED_DECL(t, x) ALIGNED_DATA(x) t
#elif defined(__GNUC__)
#define ALIGNED_DATA(x) __attribute__((aligned(x)))
#define ALIGNED_DECL(t, x) t ALIGNED_DATA(x)
#endif

struct cn_ctx {
  ALIGNED_DECL(uint8_t long_state[MEMORY], 16);
  ALIGNED_DECL(union cn_slow_hash_state state, 16);
  ALIGNED_DECL(uint8_t text[INIT_SIZE_BYTE], 16);
  ALIGNED_DECL(uint64_t a[AES_BLOCK_SIZE >> 3], 16);
  ALIGNED_DECL(uint64_t b[AES_BLOCK_SIZE >> 3], 16);
  ALIGNED_DECL(uint8_t c[AES_BLOCK_SIZE], 16);
  oaes_ctx* aes_ctx;
};

static_assert(sizeof(struct cn_ctx) == SLOW_HASH_CONTEXT_SIZE, "Invalid structure size");

static inline void ExpandAESKey256_sub1(__m128i *tmp1, __m128i *tmp2)
{
  __m128i tmp4;
  *tmp2 = _mm_shuffle_epi32(*tmp2, 0xFF);
  tmp4 = _mm_slli_si128(*tmp1, 0x04);
  *tmp1 = _mm_xor_si128(*tmp1, tmp4);
  tmp4 = _mm_slli_si128(tmp4, 0x04);
  *tmp1 = _mm_xor_si128(*tmp1, tmp4);
  tmp4 = _mm_slli_si128(tmp4, 0x04);
  *tmp1 = _mm_xor_si128(*tmp1, tmp4);
  *tmp1 = _mm_xor_si128(*tmp1, *tmp2);
}

static inline void ExpandAESKey256_sub2(__m128i *tmp1, __m128i *tmp3)
{
  __m128i tmp2, tmp4;

  tmp4 = _mm_aeskeygenassist_si128(*tmp1, 0x00);
  tmp2 = _mm_shuffle_epi32(tmp4, 0xAA);
  tmp4 = _mm_slli_si128(*tmp3, 0x04);
  *tmp3 = _mm_xor_si128(*tmp3, tmp4);
  tmp4 = _mm_slli_si128(tmp4, 0x04);
  *tmp3 = _mm_xor_si128(*tmp3, tmp4);
  tmp4 = _mm_slli_si128(tmp4, 0x04);
  *tmp3 = _mm_xor_si128(*tmp3, tmp4);
  *tmp3 = _mm_xor_si128(*tmp3, tmp2);
}

// Special thanks to Intel for helping me
// with ExpandAESKey256() and its subroutines
static inline void ExpandAESKey256(uint8_t *keybuf)
{
  __m128i tmp1, tmp2, tmp3, *keys;

  keys = (__m128i *)keybuf;

  tmp1 = _mm_load_si128((__m128i *)keybuf);
  tmp3 = _mm_load_si128((__m128i *)(keybuf+0x10));

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x01);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[2] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[3] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x02);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[4] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[5] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x04);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[6] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[7] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x08);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[8] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[9] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x10);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[10] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[11] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x20);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[12] = tmp1;
  ExpandAESKey256_sub2(&tmp1, &tmp3);
  keys[13] = tmp3;

  tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x40);
  ExpandAESKey256_sub1(&tmp1, &tmp2);
  keys[14] = tmp1;
}

  static void (*const extra_hashes_wallet[8])(const void *, size_t, char *) =
    {
        hash_extra_jh, hash_extra_skein, hash_extra_blake, hash_extra_groestl,
        hash_extra_blake, hash_extra_groestl, hash_extra_jh, hash_extra_skein
    };

  // jojapoppa, soft fork here...
  //size_t majorVersion = 1; // DON'T HARD CODE (IT'S PASSED IN)
  //static void (*const extra_hashes[8])(const void *, size_t, char *);
  //if (majorVersion <= 1) {
  static void (*const extra_hashes[8])(const void *, size_t, char *) =
    {
        //original: hash_extra_blake, hash_extra_groestl, hash_extra_jh, hash_extra_skein
        //the checksum byte and'ed (&) with 7 (0 to 7) gives index into this array to determine the hash algo used...

        hash_extra_blake, hash_extra_groestl, hash_extra_jh, hash_extra_skein,
        hash_extra_jh, hash_extra_skein, hash_extra_blake, hash_extra_groestl
    };
  //{
  //else {
  //  extra_hashes =
  //  {
  //      hash_extra_blake, hash_extra_groestl, hash_extra_jh, hash_extra_skein,
  //      hash_extra_fugue, hash_extra_argon2, hash_extra_argon2, hash_extra_gost
  //  };
  //}

#include "crypto/slow-hash.inl"
#define AESNI
#include "crypto/slow-hash.inl"

uint32_t extrahashPos(void *ctxdata) {
  return (((struct cn_ctx *)(ctxdata))->state.hs.b[0] & 7);
}

INITIALIZER(detect_aes) {
  int ecx;
#if defined(_MSC_VER)
  int cpuinfo[4];
  __cpuid(cpuinfo, 1);
  ecx = cpuinfo[2];
#else
  int a, b, d;
  __cpuid(1, a, b, ecx, d);
#endif
  cn_slow_hash_fp = (ecx & (1 << 25)) ? &cn_slow_hash_aesni : &cn_slow_hash_noaesni;
}
