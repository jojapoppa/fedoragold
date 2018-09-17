// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <stdint.h>

#include "shavite.h"

void hash_extra_shavite(const void *data, size_t length, char *hash) {
  sph_shavite256_context ctx;
  sph_shavite256_init(&ctx);
  sph_shavite256(&ctx, data, length);
  sph_shavite256(&ctx, hash);
}
