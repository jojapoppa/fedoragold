// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "StdInputStream.h"

namespace Common {

StdInputStream::StdInputStream(std::istream& in) : in(in) {
}

uint64_t StdInputStream::readSome(void* data, uint64_t size) {
  try{in.read(static_cast<char*>(data), size);}catch(...){/*do nothing*/}
  return in.gcount();
}

}
