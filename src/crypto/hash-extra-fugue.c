// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <stdint.h>

#include "hash-ops.h"
#include "sph_fugue.h"

void hash_extra_fugue(const void *data, size_t length, char *hash) {
  sph_fugue256_context S;
  sph_fugue256_init(&S);
  sph_fugue256(&S, data, length);
  sph_fugue256_close(&S, (void*)hash);
}
