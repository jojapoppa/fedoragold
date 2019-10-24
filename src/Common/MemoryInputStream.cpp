// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "MemoryInputStream.h"
#include <algorithm>
#include <cassert>
#include <cstring> // memcpy
#include <iostream>
#include <string>

namespace Common {

MemoryInputStream::MemoryInputStream(const void* buffer, size_t bufferSize) : 
buffer(static_cast<const char*>(buffer)), bufferSize(bufferSize), position(0) {}

size_t MemoryInputStream::getPosition() const {
  return position;
}

bool MemoryInputStream::endOfStream() const {
  return position == bufferSize;
}

uint64_t MemoryInputStream::readSome(void* data, uint64_t size) {
  assert(position <= bufferSize);
  uint64_t readSize = std::min(size, (uint64_t)(bufferSize - position));

  if (readSize > 0) {
    memcpy(data, buffer + position, readSize);
    position += readSize;
  }
  
  return readSize;
}

}
