// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include "Dispatcher.h"

#include <System/ErrorMessage.h>

#include "Logging/LoggerRef.h"

namespace System {

class IpAddress;

class TcpConnection {
public:
  TcpConnection();
  TcpConnection(const TcpConnection&) = delete;
  TcpConnection(TcpConnection&& other);
  ~TcpConnection();
  TcpConnection& operator=(const TcpConnection&) = delete;
  TcpConnection& operator=(TcpConnection&& other);
  std::size_t read(uint8_t* data, std::size_t size, Logging::LoggerRef &logger, bool bSynchronous=false);
  std::size_t write(const uint8_t* data, std::size_t size, Logging::LoggerRef&);
  std::pair<IpAddress, uint16_t> getPeerAddressAndPort() const;

private:
  friend class TcpConnector;
  friend class TcpListener;
  
  Dispatcher* dispatcher;
  int connection;
  ContextPair contextPair;

  TcpConnection(Dispatcher& dispatcher, int socket);

  bool testerrno(int errnm) {
    if (errnm != EAGAIN) {
      if (errnm != EWOULDBLOCK) {
        return false;
      }
    }
  return true;
  }
};

}
