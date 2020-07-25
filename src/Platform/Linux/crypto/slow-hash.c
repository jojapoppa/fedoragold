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
#include <sys/mman.h>
#include <arm_neon.h>
typedef uint8x16_t __m128i;
inline __m128i _mm_shuffle_epi32 (__m128i a, int imm)
{
  if (imm == 0) {
    return vdupq_n_s32(vgetq_lane_s32(a, 0)); 
  }

  __m128i ret;
  ret[0] = a[imm & 0x3];
  ret[1] = a[(imm >> 2) & 0x3];
  ret[2] = a[(imm >> 4) & 0x03];
  ret[3] = a[(imm >> 6) & 0x03];
  return ret;
}
#define vreinterpretq_s32_m128i(x) (x)
#define vreinterpretq_s8_m128i(x) vreinterpretq_s8_s32(x)
#define vreinterpretq_m128i_s8(x) vreinterpretq_s32_s8(x)
#define vreinterpretq_m128i_s32(x) (x)
inline __m128i _mm_aesenc_si128( __m128i v, __m128i rkey )
{
    const __attribute__((aligned(16))) __m128i zero = {0};
    return veorq_u8( vaesmcq_u8( vaeseq_u8(v, zero) ), rkey );
}
inline __m128i _mm_xor_si128(__m128i aa, __m128i bb)
{
	return vreinterpretq_m128i_s32( veorq_s32(vreinterpretq_s32_m128i(aa), vreinterpretq_s32_m128i(bb)) );
}
inline __m128i _mm_setzero_si128()
{
	return vreinterpretq_m128i_s32(vdupq_n_s32(0));
}
inline int _mm_cvtsi128_si32(__m128i aa)
{
	return vgetq_lane_s32(vreinterpretq_s32_m128i(aa), 0);
}
inline __m128i _mm_set_epi32(int i3, int i2, int i1, int i0)
{
	int32_t __attribute__((aligned(16))) data[4] = { i0, i1, i2, i3 };
	return vreinterpretq_m128i_s32(vld1q_s32(data));
}
inline void _mm_store_si128(__m128i *p, __m128i a)
{
	vst1q_s32((int32_t*) p, vreinterpretq_s32_m128i(a));
}
inline __m128i _mm_load_si128(const __m128i *pp)
{
	return vreinterpretq_m128i_s32(vld1q_s32((int32_t *)pp));
}
#define _mm_slli_si128(a1, im) \
({ \
  __m128i retn; \
  if (im <= 0) { retn = a1; } \
  else if (im > 15) { retn = _mm_setzero_si128(); } \
  else { retn = vreinterpretq_m128i_s8(vextq_s8(vdupq_n_s8(0), vreinterpretq_s8_m128i(a1), 16 - (im))); } \
  retn; \
})
static inline __attribute__((always_inline))
__m128i _mm_aeskeygenassist_si128(__m128i key, const int rcon)
{
    static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};
    uint32_t X1 = _mm_cvtsi128_si32(_mm_shuffle_epi32(key, 0x55));
    uint32_t X3 = _mm_cvtsi128_si32(_mm_shuffle_epi32(key, 0xFF));
    for( int i = 0; i < 4; ++i ) {
        ((uint8_t*)&X1)[i] = sbox[ ((uint8_t*)&X1)[i] ];
        ((uint8_t*)&X3)[i] = sbox[ ((uint8_t*)&X3)[i] ];
    }
    return _mm_set_epi32(((X3 >> 8) | (X3 << 24)) ^ rcon, X3, ((X1 >> 8) | (X1 << 24)) ^ rcon, X1);
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

#if defined(__arm__)
  cn_slow_hash_fp = &cn_slow_hash_noaesni;
#else
  cn_slow_hash_fp = (ecx & (1 << 25)) ? &cn_slow_hash_aesni : &cn_slow_hash_noaesni;
#endif
}
