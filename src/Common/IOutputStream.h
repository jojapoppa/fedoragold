// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstddef>
#include <cstdint>

namespace Common {

class IOutputStream {
public:
  virtual ~IOutputStream() { }
  virtual uint64_t writeSome(const void* data, uint64_t size) = 0;
  virtual void flush() = 0;
};

}
