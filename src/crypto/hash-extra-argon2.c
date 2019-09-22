// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2014-2018, The Aeon Project
// Copyright (c) 2018-2019, The TurtleCoin Developers

#include <stddef.h>
#include <stdint.h>

#include "hash-ops.h"
#include "argon2.h"

// Chukwa Definitions
#define CHUKWA_HASHLEN 32 // The length of the resulting hash in bytes
#define CHUKWA_SALTLEN 16 // The length of our salt in bytes
#define CHUKWA_THREADS 1 // How many threads to use at once
#define CHUKWA_ITERS 3 // How many iterations we perform as part of our slow-hash
#define CHUKWA_MEMORY 512 // This value is in KiB (0.5MB)

static bool argon2_optimization_selected = false;

void hash_extra_argon2(const void *data, size_t length, char *hash)
{
  uint8_t salt[CHUKWA_SALTLEN];
  memcpy(salt, data, sizeof(salt));

  /* If this is the first time we've called this hash function then
    we need to have the Argon2 library check to see if any of the
    available CPU instruction sets are going to help us out */
  if (!argon2_optimization_selected)
  {
    /* Call the library quick benchmark test to set which CPU
      instruction sets will be used */
    argon2_select_impl(NULL, NULL);

    argon2_optimization_selected = true;
  }

  argon2id_hash_raw(CHUKWA_ITERS, CHUKWA_MEMORY, CHUKWA_THREADS, data, 
    length, salt, CHUKWA_SALTLEN, hash, CHUKWA_HASHLEN);
}
