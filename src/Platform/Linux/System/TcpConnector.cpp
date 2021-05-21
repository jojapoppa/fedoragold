// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
//
// This file is part of Bytecoin.
//
// Bytecoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Bytecoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Bytecoin.  If not, see <http://www.gnu.org/licenses/>.

#include "TcpConnector.h"
#include <cassert>
#include <stdexcept>

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <System/InterruptedException.h>
#include <System/IpAddress.h>
#include "Dispatcher.h"
#include "ErrorMessage.h"
#include "TcpConnection.h"

namespace System {

namespace {

struct TcpConnectorContextExt : public OperationContext {
  int connection;
};

}

TcpConnector::TcpConnector() : dispatcher(nullptr) {
}

TcpConnector::TcpConnector(Dispatcher& dispatcher) : dispatcher(&dispatcher), context(nullptr) {
}

TcpConnector::TcpConnector(TcpConnector&& other) : dispatcher(other.dispatcher) {
  if (other.dispatcher != nullptr) {
    assert(other.context == nullptr);
    context = nullptr;
    other.dispatcher = nullptr;
  }
}

TcpConnector::~TcpConnector() {
}

TcpConnector& TcpConnector::operator=(TcpConnector&& other) {
  dispatcher = other.dispatcher;
  if (other.dispatcher != nullptr) {
    assert(other.context == nullptr);
    context = nullptr;
    other.dispatcher = nullptr;
  }

  return *this;
}

TcpConnection TcpConnector::connect(const IpAddress& address, uint16_t port) {
  assert(dispatcher != nullptr);
  assert(context == nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  std::string message;
  int connection = -1;
  try{connection=::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);}catch(...){connection=-1;}
  if (connection == -1) {
    message = "socket failed, " + lastErrorMessage();
  } else {
    sockaddr_in bindAddress;
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_port = 0;
    bindAddress.sin_addr.s_addr = INADDR_ANY;

    int bres=-1;
    try{bres=bind(connection, reinterpret_cast<sockaddr*>(&bindAddress), sizeof bindAddress);}
      catch(...){bres=-1;}

    if (bres != 0) {
      message = "bind failed, " + lastErrorMessage();
    } else {
      int flags = -1;
      try{flags=fcntl(connection, F_GETFL, 0);}catch(...){flags=-1;}

      int flags2 = -1;
      if (flags != -1) try{flags2=fcntl(connection, F_SETFL,
        flags | O_NONBLOCK);}catch(...){flags2=-1;}

      if (flags == -1 || flags2 == -1) {
        message = "fcntl failed, " + lastErrorMessage();
      } else {
        sockaddr_in addressData;
        addressData.sin_family = AF_INET;

        bool bAddr = true;
        try {
          addressData.sin_port = htons(port);
          addressData.sin_addr.s_addr = htonl(address.getValue());
        } catch(...) { bAddr = false; }

        int result = -1;
        try{result=::connect(connection, reinterpret_cast<sockaddr *>(&addressData),
          sizeof addressData);}catch(...){result=-1;}
        if (result == -1 || !bAddr) {
          if (errno == EINPROGRESS) {

            ContextPair contextPair;
            TcpConnectorContextExt connectorContext;
            connectorContext.interrupted = false;
            connectorContext.context = dispatcher->getCurrentContext();
            connectorContext.connection = connection;

            contextPair.readContext = nullptr;
            contextPair.writeContext = &connectorContext;

            epoll_event connectEvent;
            connectEvent.events = EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLONESHOT;
            connectEvent.data.ptr = &contextPair;

            int eres=-1;
            try{eres=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_ADD,
              connection, &connectEvent);}catch(...){eres=-1;}

            if (eres == -1) {
              message = "epoll_ctl failed, " + lastErrorMessage();
            } else {
              context = &connectorContext;
              dispatcher->getCurrentContext()->interruptProcedure = [&] {
                TcpConnectorContextExt* connectorContext1 = static_cast<TcpConnectorContextExt*>(context);
                if (!connectorContext1->interrupted) {

                  int cres=-1;
                  try{cres=close(connectorContext1->connection);}catch(...){cres=-1;}

                  if (cres == -1) {
                    throw std::runtime_error("TcpListener::stop, close failed, " + lastErrorMessage());
                  }

                  connectorContext1->interrupted = true;
                  dispatcher->pushContext(connectorContext1->context);
                }
              };

              dispatcher->dispatch();
              dispatcher->getCurrentContext()->interruptProcedure = nullptr;
              assert(dispatcher != nullptr);
              assert(connectorContext.context == dispatcher->getCurrentContext());
              assert(contextPair.readContext == nullptr);
              assert(context == &connectorContext);
              context = nullptr;
              connectorContext.context = nullptr;
              if (connectorContext.interrupted) {
                throw InterruptedException();
              }

              int pres=-1;
              try{pres=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_DEL, connection,
                NULL);}catch(...){pres=-1;}

              if (pres == -1) {
                message = "epoll_ctl failed, " + lastErrorMessage();
              } else {
                if((connectorContext.events & (EPOLLERR | EPOLLHUP)) != 0) {

                  int result = -1;
                  try{result=close(connection);}catch(...){result=-1;}
                  assert(result != -1);

                  throw std::runtime_error("TcpConnector::connect, connection failed");
                }

                int retval = -1;
                socklen_t retValLen = sizeof(retval);
                int s = -1;
                try{s=getsockopt(connection, SOL_SOCKET, SO_ERROR, &retval,
                  &retValLen);}catch(...){s=-1;}

                if (s != 0) {
                  message = "";
                  if (retval != 0) {
                    if (errno == EBADF || errno == ENOTSOCK)
                      message = "bad sockfd : ";
                    else if (errno == EFAULT)
                      message = "connection not part of valid address space : ";
                    else if (errno == EINVAL)
                      message = "invalid value passed to connection : ";
                  }
                  message = message + "getsockopt failed: " + lastErrorMessage();
                } else {
                  return TcpConnection(*dispatcher, connection);
                }
              }
            }
          }
        } else {
          return TcpConnection(*dispatcher, connection);
        }
      }
    }

    int result = -1;
    try{result=close(connection);}catch(...){result=-1;}
    assert(result != -1);
  }

  throw std::runtime_error("TcpConnector::connect, "+message);
}

}
