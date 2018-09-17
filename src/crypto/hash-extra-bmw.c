// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <stdint.h>

#include "bmw.h"

void hash_extra_bmw(const void *data, size_t length, char *hash) {
  sph_bmw256_context ctx;
  sph_bmw256_init(&ctx);
  sph_bmw256(&ctx, data, length);
  sph_bmw256(&ctx, hash);
}
