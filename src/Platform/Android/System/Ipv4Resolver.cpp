// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "Ipv4Resolver.h"
#include <cassert>
#include <random>
#include <stdexcept>

#include <netdb.h>
#include <netinet/in.h>

#include <System/Dispatcher.h>
#include <System/ErrorMessage.h>
#include <System/InterruptedException.h>
#include <System/IpAddress.h>

namespace System {

Ipv4Resolver::Ipv4Resolver() : dispatcher(nullptr) {
}

Ipv4Resolver::Ipv4Resolver(Dispatcher& dispatcher) : dispatcher(&dispatcher) {
}

Ipv4Resolver::Ipv4Resolver(Ipv4Resolver&& other) : dispatcher(other.dispatcher) {
  if (dispatcher != nullptr) {
    other.dispatcher = nullptr;
  }
}

Ipv4Resolver::~Ipv4Resolver() {
}

Ipv4Resolver& Ipv4Resolver::operator=(Ipv4Resolver&& other) {
  dispatcher = other.dispatcher;
  if (dispatcher != nullptr) {
    other.dispatcher = nullptr;
  }

  return *this;
}

IpAddress Ipv4Resolver::resolve(const std::string& host) {
  assert(dispatcher != nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  addrinfo hints = { 0, AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL };
  addrinfo* addressInfos;
  int result = getaddrinfo(host.c_str(), NULL, &hints, &addressInfos);
  if (result != 0) {
    throw std::runtime_error("Ipv4Resolver::resolve, getaddrinfo failed, " + errorMessage(result));
  }
  
  std::size_t count = 0;
  for (addrinfo* addressInfo = addressInfos; addressInfo != nullptr; addressInfo = addressInfo->ai_next) {
    ++count;
  }

  std::mt19937 generator{ std::random_device()() };
  std::size_t index = std::uniform_int_distribution<std::size_t>(0, count - 1)(generator);
  addrinfo* addressInfo = addressInfos;
  for (std::size_t i = 0; i < index; ++i) {
    addressInfo = addressInfo->ai_next;
  }

  //jojapoppa, need to set this flag, and double check differences in addressinfo structure

  bool isIpv4 = true;
  IpAddress *address;
 
  if (isIpv4) { 
    address = new IpAddress(ntohl(reinterpret_cast<sockaddr_in*>(addressInfo->ai_addr)->sin_addr.s_addr));
  } else {
    address = new IpAddress(reinterpret_cast<sockaddr_in6*>(addressInfo->ai_addr)->sin6_addr);
  }

  freeaddrinfo(addressInfo);
  return *address;
}

}
