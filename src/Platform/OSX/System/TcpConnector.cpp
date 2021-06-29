// Copyright (c) 2012-2016, The CryptoNote developers, The Bytecoin developers
//
// This file is part of Karbo.
//
// Karbo is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Karbo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Karbo.  If not, see <http://www.gnu.org/licenses/>.

#include "TcpConnector.h"
#include <cassert>

#include <fcntl.h>
#include <netdb.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>

#include <System/InterruptedException.h>
#include <System/IpAddress.h>
#include "Dispatcher.h"
#include "ErrorMessage.h"
#include "TcpConnection.h"

namespace System {

namespace {

struct ConnectorContext : public OperationContext {
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
  assert(dispatcher == nullptr || context == nullptr);
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
    if (bind(connection, reinterpret_cast<sockaddr*>(&bindAddress), sizeof bindAddress) != 0) {
      message = "bind failed, " + lastErrorMessage();
    } else {
      int flags = -1;
      try{flags=fcntl(connection, F_GETFL, 0);}catch(...){flags=-1;}
      int flags2 = -1;
      if (flags != -1) try{flags2=fcntl(connection, F_SETFL, flags | O_NONBLOCK);}catch(...){flags2=-1;}
      if (flags == -1 || flags2 == -1) {
        message = "fcntl failed, " + lastErrorMessage();
      } else {
        sockaddr_in addressData;
        addressData.sin_family = AF_INET;
        addressData.sin_port = htons(port);
        addressData.sin_addr.s_addr = htonl(address.getValue());
        int result = -1;
        try{result=::connect(connection, reinterpret_cast<sockaddr *>(&addressData), sizeof addressData);}
          catch(...){result=-1;}
        if (result == -1) {
          if (errno == EINPROGRESS) {
            ConnectorContext connectorContext;
            connectorContext.context = dispatcher->getCurrentContext();
            connectorContext.interrupted = false;
            connectorContext.connection = connection;

            struct kevent event;
            EV_SET(&event, connection, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &connectorContext);
            int kresult=-1;
            try{kresult=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){kresult=-1;}
            if (kresult == -1) {
              message = "kevent failed, " + lastErrorMessage();
            } else {
              context = &connectorContext;
              dispatcher->getCurrentContext()->interruptProcedure = [&] {
                assert(dispatcher != nullptr);
                assert(context != nullptr);
                ConnectorContext* connectorContext = static_cast<ConnectorContext*>(context);
                if (!connectorContext->interrupted) {
                  int cresult=-1;
                  try{cresult=close(connectorContext->connection);}catch(...){cresult=-1;}
                  if (cresult == -1) {
                    //throw std::runtime_error("TcpListener::stop, close failed, " + lastErrorMessage());
                    std::cerr << "TcpListener::stop, close failed, " << lastErrorMessage() << std::endl;
                  }
                  
                  dispatcher->pushContext(connectorContext->context);
                  connectorContext->interrupted = true;
                }                
              };
              
              dispatcher->dispatch();
              dispatcher->getCurrentContext()->interruptProcedure = nullptr;
              assert(dispatcher != nullptr);
              assert(connectorContext.context == dispatcher->getCurrentContext());
              assert(context == &connectorContext);
              context = nullptr;
              connectorContext.context = nullptr;
              if (connectorContext.interrupted) {
                throw InterruptedException();
              }

              struct kevent event;
              EV_SET(&event, connection, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL);

              int kresult=-1;
              try{kresult=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){kresult=-1;}
              if (kresult == -1) {
                message = "kevent failed, " + lastErrorMessage();
              } else {
                int retval = -1;
                socklen_t retValLen = sizeof(retval);
                int s = -1;
                try{s=getsockopt(connection, SOL_SOCKET, SO_ERROR, &retval, &retValLen);}catch(...){s=-1;}
                if (s == -1) {
                  message = "getsockopt failed, " + lastErrorMessage();
                } else {
                  if (retval != 0) {
                    message = "getsockopt failed, " + lastErrorMessage();
                  } else {
                    return TcpConnection(*dispatcher, connection);
                  }
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
    assert(result != -1);;
  }

  //normal processing... do don't print anything out...
  //throw std::runtime_error("TcpConnector::connect, " + message);
  //std::cerr << "TcpConnector::connect, " << message << std::endl;
  return TcpConnection(*dispatcher, connection);
}

}
