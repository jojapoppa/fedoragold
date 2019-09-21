// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <stdint.h>

#include "hash-ops.h"
#include "sph_gost.h"

void hash_extra_fugue(const void *data, size_t length, char *hash) {
  sph_gost256_context S;
  sph_gost256_init(&S);
  sph_gost256(&S, data, length);
  sph_gost256_close(&S, (void*)hash);
}
