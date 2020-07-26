// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netdb.h>
#endif

namespace System {

class IpAddress {
public:
  explicit IpAddress(uint32_t value);
  //explicit IpAddress(struct in6_addr addr6);
  explicit IpAddress(const std::string& dottedDecimal);
  bool operator!=(const IpAddress& other) const;
  bool operator==(const IpAddress& other) const;
  uint32_t getValue() const;
  bool isLoopback() const;
  bool isPrivate() const;
  std::string toDottedDecimal() const;

private:
  uint32_t value;
};

}
