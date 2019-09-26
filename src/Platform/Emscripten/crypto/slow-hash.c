// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2014-2018, The Aeon Project
// Copyright (c) 2018, The TurtleCoin Developers
//
// Please see the included LICENSE file for more information.

/* This file contains the portable version of the slow-hash routines
   for the CryptoNight hashing algorithm */

#define FORCE_USE_HEAP 1

#if !(!defined NO_AES && (defined(__arm__) || defined(__aarch64__))) && !(!defined NO_AES && (defined(__x86_64__) || (defined(_MSC_VER) && defined(_WIN64))))
//  #pragma message ("info: Using slow-hash-portable.c")

  #include "crypto/slow-hash-common.h"
  #include "crypto/aesb.h"


void slow_hash_allocate_state(void)
{
    // Do nothing, this is just to maintain compatibility with the upgraded slow-hash.c
    return;
}

void slow_hash_free_state(void)
{
    // As above
    return;
}

  #if defined(__GNUC__)
    #define RDATA_ALIGN16 __attribute__ ((aligned(16)))
    #define STATIC static
    #define INLINE inline
  #else /* defined(__GNUC__) */
    #define RDATA_ALIGN16
    #define STATIC static
    #define INLINE
  #endif /* defined(__GNUC__) */

  #define U64(x) ((uint64_t *) (x))

//static void (*const extra_hashes[8])(const void *, size_t, char *) = {};

//extern void aesb_single_round(const uint8_t *in, uint8_t*out, const uint8_t *expandedKey);
//extern void aesb_pseudo_round(const uint8_t *in, uint8_t *out, const uint8_t *expandedKey);

static inline uint64_t hi_dword(uint64_t val) {
  return val >> 32;
}

static inline uint64_t lo_dword(uint64_t val) {
  return val & 0xFFFFFFFF;
}

static inline uint64_t mul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi) {
  // multiplier   = ab = a * 2^32 + b
  // multiplicand = cd = c * 2^32 + d
  // ab * cd = a * c * 2^64 + (a * d + b * c) * 2^32 + b * d
  uint64_t a = hi_dword(multiplier);
  uint64_t b = lo_dword(multiplier);
  uint64_t c = hi_dword(multiplicand);
  uint64_t d = lo_dword(multiplicand);

  uint64_t ac = a * c;
  uint64_t ad = a * d;
  uint64_t bc = b * c;
  uint64_t bd = b * d;

  uint64_t adbc = ad + bc;
  uint64_t adbc_carry = adbc < ad ? 1 : 0;

  // multiplier * multiplicand = product_hi * 2^64 + product_lo
  uint64_t product_lo = bd + (adbc << 32);
  uint64_t product_lo_carry = product_lo < bd ? 1 : 0;
  *product_hi = ac + (adbc >> 32) + (adbc_carry << 32) + product_lo_carry;
  assert(ac <= *product_hi);

  return product_lo;
}

static void mul(const uint8_t* a, const uint8_t* b, uint8_t* res)
{
    uint64_t a0, b0;
    uint64_t hi, lo;

    a0 = SWAP64LE(((uint64_t*)a)[0]);
    b0 = SWAP64LE(((uint64_t*)b)[0]);
    lo = mul128(a0, b0, &hi);
    ((uint64_t*)res)[0] = SWAP64LE(hi);
    ((uint64_t*)res)[1] = SWAP64LE(lo);
}

static void sum_half_blocks(uint8_t* a, const uint8_t* b)
{
    uint64_t a0, a1, b0, b1;

    a0 = SWAP64LE(((uint64_t*)a)[0]);
    a1 = SWAP64LE(((uint64_t*)a)[1]);
    b0 = SWAP64LE(((uint64_t*)b)[0]);
    b1 = SWAP64LE(((uint64_t*)b)[1]);
    a0 += b0;
    a1 += b1;
    ((uint64_t*)a)[0] = SWAP64LE(a0);
    ((uint64_t*)a)[1] = SWAP64LE(a1);
}

static void copy_block(uint8_t* dst, const uint8_t* src)
{
    memcpy(dst, src, AES_BLOCK_SIZE);
}

static void swap_blocks(uint8_t *a, uint8_t *b)
{
    uint64_t t[2];
    U64(t)[0] = U64(a)[0];
    U64(t)[1] = U64(a)[1];
    U64(a)[0] = U64(b)[0];
    U64(a)[1] = U64(b)[1];
    U64(b)[0] = U64(t)[0];
    U64(b)[1] = U64(t)[1];
}

static void xor_blocks(uint8_t* a, const uint8_t* b)
{
    size_t i;
    for (i = 0; i < AES_BLOCK_SIZE; i++)
    {
        a[i] ^= b[i];
    }
}

static void xor64(uint8_t* left, const uint8_t* right)
{
    size_t i;
    for (i = 0; i < 8; ++i)
    {
        left[i] ^= right[i];
    }
}

void cn_slow_hash(size_t majorVersion, const void *data, size_t length, char *hash, int light, int variant, int prehashed, uint32_t page_size, uint32_t scratchpad, uint32_t iterations)
{
  uint32_t init_rounds = (scratchpad / INIT_SIZE_BYTE);
  uint32_t aes_rounds = (iterations / 2);
  size_t lightFlag = (light ? 2: 1);

  uint8_t text[INIT_SIZE_BYTE];
  uint8_t a[AES_BLOCK_SIZE];
  uint8_t b[AES_BLOCK_SIZE * 2];
  uint8_t c[AES_BLOCK_SIZE];
  uint8_t c1[AES_BLOCK_SIZE];
  uint8_t d[AES_BLOCK_SIZE];
  RDATA_ALIGN16 uint8_t expandedKey[256];

  union cn_slow_hash_state state;

  size_t i, j;
  uint8_t *p = NULL;
  oaes_ctx *aes_ctx;

  // jojapoppa, soft fork here...
  //size_t majorVersion = 1; // DON'T HARD CODE (IT'S PASSED IN)
  //static void (*const extra_hashes[8])(const void *, size_t, char *);
  //if (majorVersion <= 1) {
  static void (*const extra_hashes[8])(const void *, size_t, char *) =
    {
        //original: hash_extra_blake, hash_extra_groestl, hash_extra_jh, hash_extra_skein
        //the checksum byte and'ed (&) with 7 (0 to 7) gives index into this array to determine the hash algo used...

        // 1/8 hash overlap algo (requires hard fork - diff algo above given height would be required
        // hash_extra_blake, hash_extra_jh, hash_extra_skein, hash_extra_groestl,
        // hash_extra_groestl, hash_extra_skein, hash_extra_blake, hash_extra_jh

        hash_extra_jh, hash_extra_skein, hash_extra_blake, hash_extra_groestl,
        hash_extra_blake, hash_extra_groestl, hash_extra_jh, hash_extra_skein
    };
  //{
  //else {
  //  extra_hashes =
  //  {
  //      hash_extra_jh, hash_extra_skein, hash_extra_blake, hash_extra_groestl,
  //      hash_extra_fugue, hash_extra_argon2, hash_extra_argon2, hash_extra_gost
  //  };
  //}

  #ifndef FORCE_USE_HEAP
    uint8_t long_state[page_size];
  #else /* FORCE_USE_HEAP */
    #pragma message ("warning: ACTIVATING FORCE_USE_HEAP IN slow-hash-portable.c")
    uint8_t *long_state = (uint8_t *)malloc(page_size);
  #endif /* FORCE_USE_HEAP */

    if (prehashed)
    {
        memcpy(&state.hs, data, length);
    } else {
        hash_process(&state.hs, data, length);
    }

    memcpy(text, state.init, INIT_SIZE_BYTE);

    aes_ctx = (oaes_ctx *) oaes_alloc();
    oaes_key_import_data(aes_ctx, state.hs.b, AES_KEY_SIZE);

    VARIANT1_PORTABLE_INIT();
    VARIANT2_PORTABLE_INIT();

    // use aligned data
    memcpy(expandedKey, aes_ctx->key->exp_data, aes_ctx->key->exp_data_len);

    for(i = 0; i < init_rounds; i++)
    {
        for(j = 0; j < INIT_SIZE_BLK; j++)
            aesb_pseudo_round(&text[AES_BLOCK_SIZE * j], &text[AES_BLOCK_SIZE * j], expandedKey);
        memcpy(&long_state[i * INIT_SIZE_BYTE], text, INIT_SIZE_BYTE);
    }

    U64(a)[0] = U64(&state.k[0])[0] ^ U64(&state.k[32])[0];
    U64(a)[1] = U64(&state.k[0])[1] ^ U64(&state.k[32])[1];
    U64(b)[0] = U64(&state.k[16])[0] ^ U64(&state.k[48])[0];
    U64(b)[1] = U64(&state.k[16])[1] ^ U64(&state.k[48])[1];

    for(i = 0; i < aes_rounds; i++)
    {
    #define MASK(div) ((uint32_t)(((page_size / AES_BLOCK_SIZE) / (div) - 1) << 4))
    #define state_index(x,div) ((*(uint32_t *) x) & MASK(div))

      // Iteration 1
      j = state_index(a,lightFlag);
      p = &long_state[j];
      aesb_single_round(p, p, a);
      copy_block(c1, p);

      VARIANT2_PORTABLE_SHUFFLE_ADD(long_state, j);
      xor_blocks(p, b);
      VARIANT1_1(p);

      // Iteration 2
      j = state_index(c1,lightFlag);
      p = &long_state[j];
      copy_block(c, p);

      VARIANT2_PORTABLE_INTEGER_MATH(c, c1);
      mul(c1, c, d);
      VARIANT2_2_PORTABLE();
      VARIANT2_PORTABLE_SHUFFLE_ADD(long_state, j);
      sum_half_blocks(a, d);
      swap_blocks(a, c);
      xor_blocks(a, c);
      VARIANT1_2(c + 8);
      copy_block(p, c);

      if (variant >= 2) {
          copy_block(b + AES_BLOCK_SIZE, b);
      }

      copy_block(b, c1);
    }

    memcpy(text, state.init, INIT_SIZE_BYTE);
    oaes_key_import_data(aes_ctx, &state.hs.b[32], AES_KEY_SIZE);
    memcpy(expandedKey, aes_ctx->key->exp_data, aes_ctx->key->exp_data_len);

    for(i = 0; i < init_rounds; i++)
    {
        for(j = 0; j < INIT_SIZE_BLK; j++)
        {
            xor_blocks(&text[j * AES_BLOCK_SIZE], &long_state[i * INIT_SIZE_BYTE + j * AES_BLOCK_SIZE]);
            aesb_pseudo_round(&text[AES_BLOCK_SIZE * j], &text[AES_BLOCK_SIZE * j], expandedKey);
        }
    }

    oaes_free((OAES_CTX **) &aes_ctx);
    memcpy(state.init, text, INIT_SIZE_BYTE);
    hash_permutation(&state.hs);
    extra_hashes[state.hs.b[0] & 7](&state, 200, hash);
    oaes_free((OAES_CTX **) &aes_ctx);

    #ifdef FORCE_USE_HEAP
    free(long_state);
    #endif /* FORCE_USE_HEAP */
}

/* Standard for Cryptonight v1 */
#define CN_PAGE_SIZE                    2097152    /* 2 MiB */
#define CN_SCRATCHPAD                   2097152
#define CN_ITERATIONS                   1048576

/* note: passed contextData parameter below is not used in this "portable" version, so it is ignored here */
void cn_slow_hash_f(size_t majorVersion, void * contextData, const void * data, size_t length, void * hash){
    // not cryptonight light so 'light' flag is zero, cn variant is 1
    // set 'prehashed' to false for now...

    cn_slow_hash(majorVersion, data, length, (char *)hash, 0, 1, 0, (uint32_t)CN_PAGE_SIZE, (uint32_t)CN_SCRATCHPAD, CN_ITERATIONS);
}

#endif