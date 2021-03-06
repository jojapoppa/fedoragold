// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "StringOutputStream.h"

namespace Common {

StringOutputStream::StringOutputStream(std::string& out) : out(out) {
}

uint64_t StringOutputStream::writeSome(const void* data, uint64_t size) {
  out.append(static_cast<const char*>(data), size);
  return size;
}

}
