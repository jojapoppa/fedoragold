// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <memory>

namespace CryptoNote {

class IInputStream {
public:
  virtual uint64_t read(char* data, uint64_t size) = 0;
};

class IOutputStream {
public:
  virtual void write(const char* data, uint64_t size) = 0;
};

}
