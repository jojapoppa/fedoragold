// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "IOutputStream.h"

namespace Common {

class StringOutputStream : public IOutputStream {
public:
  StringOutputStream(std::string& out);
  uint64_t writeSome(const void* data, uint64_t size) override;

  void flush() override { /* do nothing */ }

private:
  std::string& out;
};

}
