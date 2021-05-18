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

#include "TcpConnection.h"

#include <arpa/inet.h>
#include <cassert>
#include <sys/epoll.h>
#include <unistd.h>

#include <System/ErrorMessage.h>
#include <System/InterruptedException.h>
#include <System/IpAddress.h>

namespace System {

TcpConnection::TcpConnection() : dispatcher(nullptr) {
}

TcpConnection::TcpConnection(TcpConnection&& other) : dispatcher(other.dispatcher) {
  if (other.dispatcher != nullptr) {
    assert(other.contextPair.writeContext == nullptr);
    assert(other.contextPair.readContext == nullptr);
    connection = other.connection;
    contextPair = other.contextPair;
    other.dispatcher = nullptr;
  }
}

TcpConnection::~TcpConnection() {
  if (dispatcher != nullptr) {
    assert(contextPair.readContext == nullptr);
    assert(contextPair.writeContext == nullptr);
    int result = -1;
    try{result=close(connection);}catch(...){result=-1;}
    if (result) {}
    assert(result != -1);
  }
}

TcpConnection& TcpConnection::operator=(TcpConnection&& other) {
  if (dispatcher != nullptr) {
    assert(contextPair.readContext == nullptr);
    assert(contextPair.writeContext == nullptr);
    int result = -1;
    try{result=close(connection);}catch(...){result=-1;}
    if (result == -1) {
      throw std::runtime_error("TcpConnection::operator=, close failed, " + lastErrorMessage());
    }
  }

  dispatcher = other.dispatcher;
  if (other.dispatcher != nullptr) {
    assert(other.contextPair.readContext == nullptr);
    assert(other.contextPair.writeContext == nullptr);
    connection = other.connection;
    contextPair = other.contextPair;
    other.dispatcher = nullptr;
  }

  return *this;
}

size_t TcpConnection::read(uint8_t* data, size_t size, Logging::LoggerRef &logger, bool bSynchronous) {
  assert(dispatcher != nullptr);
  assert(contextPair.readContext == nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  // Note: the bSynchronous flag is not needed on Linux...

  std::string message;
  ssize_t transferred = -1;
  try{transferred=::recv(connection, (void *)data, size, 0);}catch(...){transferred=-1;}
  if (transferred == -1) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlogical-op"
    if (errno != EAGAIN  && errno != EWOULDBLOCK) {
#pragma GCC diagnostic pop
      message = "recv failed, " + lastErrorMessage();
    } else {
      epoll_event connectionEvent;
      OperationContext operationContext;
      operationContext.interrupted = false;
      operationContext.context = dispatcher->getCurrentContext();
      contextPair.readContext = &operationContext;
      connectionEvent.data.ptr = &contextPair;

      if(contextPair.writeContext != nullptr) {
        connectionEvent.events = EPOLLIN | EPOLLOUT | EPOLLONESHOT;
      } else {
        connectionEvent.events = EPOLLIN | EPOLLONESHOT;
      }

      int eresult = -1;
      try{eresult=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD, connection, &connectionEvent);}
        catch(...){eresult=-1;}
      if (eresult == -1) {
        message = "epoll_ctl failed, " + lastErrorMessage();
      } else {
        dispatcher->getCurrentContext()->interruptProcedure = [&]() {
            assert(dispatcher != nullptr);
            assert(contextPair.readContext != nullptr);
            epoll_event connectionEvent;
            connectionEvent.events = EPOLLONESHOT;
            connectionEvent.data.ptr = nullptr;

            int presult = -1;
            try{presult=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD, connection,
              &connectionEvent);}catch(...){presult=-1;}

            if (presult == -1) {
              throw std::runtime_error("TcpConnection::read, interrupt procedure, epoll_ctl failed, " + lastErrorMessage());
            }

            contextPair.readContext->interrupted = true;
            dispatcher->pushContext(contextPair.readContext->context);
        };

        dispatcher->dispatch();
        dispatcher->getCurrentContext()->interruptProcedure = nullptr;
        assert(dispatcher != nullptr);
        assert(operationContext.context == dispatcher->getCurrentContext());
        assert(contextPair.readContext == &operationContext);

        if (operationContext.interrupted) {
          contextPair.readContext = nullptr;
          throw InterruptedException();
        }

        contextPair.readContext = nullptr;
        if(contextPair.writeContext != nullptr) { //write is presented, rearm
          epoll_event connectionEvent;
          connectionEvent.events = EPOLLOUT | EPOLLONESHOT;
          connectionEvent.data.ptr = &contextPair;

          int rslt=-1;
          try{rslt=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD,
            connection, &connectionEvent);}catch(...){rslt=-1;}
          if (rslt == -1) {
            message = "epoll_ctl failed, " + lastErrorMessage();
            throw std::runtime_error("TcpConnection::read");
          }
        }

        if((operationContext.events & (EPOLLERR | EPOLLHUP)) != 0) {
          throw std::runtime_error("TcpConnection::read");
        }

        ssize_t transferred = -1;
        try{transferred=::recv(connection, (void *)data, size, 0);}catch(...){transferred=-1;}
        if (transferred == -1) {
          message = "recv failed, " + lastErrorMessage();
        } else {
          assert(transferred <= static_cast<ssize_t>(size));
          return transferred;
        }
      }
    }

    throw std::runtime_error("TcpConnection::read, "+ message);
  }

  assert(transferred <= static_cast<ssize_t>(size));
  return transferred;
}

std::size_t TcpConnection::write(const uint8_t* data, size_t size, Logging::LoggerRef& logger) {
  assert(dispatcher != nullptr);
  assert(contextPair.writeContext == nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  std::string message;
  if(size == 0) {
    int sresult=-1;
    try{sresult=shutdown(connection, SHUT_WR);}catch(...){sresult=-1;}
    if(sresult == -1) {
      throw std::runtime_error("TcpConnection::write, shutdown failed, " + lastErrorMessage());
    }

    return 0;
  }

  ssize_t transferred = -1;
  try{transferred=::send(connection,
    (void *)data, size, MSG_NOSIGNAL);}catch(...){transferred=-1;}
  if (transferred == -1) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlogical-op"
    if (errno != EAGAIN  && errno != EWOULDBLOCK) {
#pragma GCC diagnostic pop
      message = "send failed, " + lastErrorMessage();
    } else {
      epoll_event connectionEvent;
      OperationContext operationContext;
      operationContext.interrupted = false;
      operationContext.context = dispatcher->getCurrentContext();
      contextPair.writeContext = &operationContext;
      connectionEvent.data.ptr = &contextPair;

      if(contextPair.readContext != nullptr) {
        connectionEvent.events = EPOLLIN | EPOLLOUT | EPOLLONESHOT;
      } else {
        connectionEvent.events = EPOLLOUT | EPOLLONESHOT;
      }

      int oresult=-1;
      try{oresult=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD,
        connection, &connectionEvent);}catch(...){oresult=-1;}

      if (oresult == -1) {
        message = "epoll_ctl failed, " + lastErrorMessage();
      } else {
        dispatcher->getCurrentContext()->interruptProcedure = [&]() {
            assert(dispatcher != nullptr);
            assert(contextPair.writeContext != nullptr);
            epoll_event connectionEvent;
            connectionEvent.events = EPOLLONESHOT;
            connectionEvent.data.ptr = nullptr;

            int tresult=-1;
            try{tresult=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD,
              connection, &connectionEvent);}catch(...){tresult=-1;}

            if (tresult == -1) {
              throw std::runtime_error("TcpConnection::write, interrupt procedure, epoll_ctl failed, " + lastErrorMessage());
            }

            contextPair.writeContext->interrupted = true;
            dispatcher->pushContext(contextPair.writeContext->context);
        };

        dispatcher->dispatch();
        dispatcher->getCurrentContext()->interruptProcedure = nullptr;
        assert(dispatcher != nullptr);
        assert(operationContext.context == dispatcher->getCurrentContext());
        assert(contextPair.writeContext == &operationContext);

        if (operationContext.interrupted) {
          contextPair.writeContext = nullptr;
          throw InterruptedException();
        }

        contextPair.writeContext = nullptr;
        if(contextPair.readContext != nullptr) { //read is presented, rearm
          epoll_event connectionEvent;
          connectionEvent.events = EPOLLIN | EPOLLONESHOT;
          connectionEvent.data.ptr = &contextPair;

          int lreslt=-1;
          try{lreslt=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD,
            connection, &connectionEvent);}catch(...){lreslt=-1;}

          if (lreslt == -1) {
            message = "epoll_ctl failed, " + lastErrorMessage();
            throw std::runtime_error("TcpConnection::write, " + message);
          }
        }

        if((operationContext.events & (EPOLLERR | EPOLLHUP)) != 0) {
          throw std::runtime_error("TcpConnection::write, events & (EPOLLERR | EPOLLHUP) != 0");
        }

        ssize_t transferred = -1;
        try{transferred=::send(connection, (void *)data, size, 0);}catch(...){transferred=-1;}
        if (transferred == -1) {
          message = "send failed, "  + lastErrorMessage();
        } else {
          assert(transferred <= static_cast<ssize_t>(size));
          return transferred;
        }
      }
    }

    throw std::runtime_error("TcpConnection::write, " + message);
  }

  assert(transferred <= static_cast<ssize_t>(size));
  return transferred;
}

std::pair<IpAddress, uint16_t> TcpConnection::getPeerAddressAndPort() const {
  sockaddr_in addr;
  socklen_t size = sizeof(addr);
  if (getpeername(connection, reinterpret_cast<sockaddr*>(&addr), &size) != 0) {
    throw std::runtime_error("TcpConnection::getPeerAddress, getpeername failed, " + lastErrorMessage());
  }

  assert(size == sizeof(sockaddr_in));
  return std::make_pair(IpAddress(htonl(addr.sin_addr.s_addr)), htons(addr.sin_port));
}

TcpConnection::TcpConnection(Dispatcher& dispatcher, int socket) : dispatcher(&dispatcher), connection(socket) {
  contextPair.readContext = nullptr;
  contextPair.writeContext = nullptr;
  epoll_event connectionEvent;
  connectionEvent.events = EPOLLONESHOT;
  connectionEvent.data.ptr = nullptr;

  int eresult=-1;
  try{eresult=epoll_ctl(dispatcher.getEpoll(), EPOLL_CTL_ADD, socket,
    &connectionEvent);}catch(...){eresult=-1;}

  if (eresult == -1) {
    throw std::runtime_error("TcpConnection::TcpConnection, epoll_ctl failed, " + lastErrorMessage());
  }
}

}
