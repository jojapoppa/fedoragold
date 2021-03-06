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

#include "TcpListener.h"
#include <cassert>
#include <stdexcept>

#include <fcntl.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

#include "Dispatcher.h"
#include "TcpConnection.h"
#include <System/ErrorMessage.h>
#include <System/InterruptedException.h>
#include <System/IpAddress.h>

namespace System {

TcpListener::TcpListener() : dispatcher(nullptr) {
}

TcpListener::TcpListener(Dispatcher& dispatcher, const IpAddress& addr, uint16_t port) : dispatcher(&dispatcher) {
  std::string message;
  listener = -1;
  try{listener=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);}catch(...){listener=-1;}
  if (listener == -1) {
    message = "socket failed, " + lastErrorMessage();
  } else {
    int flags = -1;
    try{flags=fcntl(listener, F_GETFL, 0);}catch(...){flags=-1;}
    int flags2 = -1;
    if (flags != -1) {try{flags2=fcntl(listener, F_SETFL, 
      flags | O_NONBLOCK);}catch(...){flags2=-1;}}

    if (flags == -1 || flags2 == -1) {
      message = "fcntl failed, " + lastErrorMessage();
    } else {
      int on = 1;
      int sres = -1;
      try{sres=setsockopt(listener, SOL_SOCKET,
        SO_REUSEADDR|SO_REUSEPORT, &on, sizeof on);}catch(...){sres=-1;}

      if (sres == -1) {
        message = "setsockopt failed, " + lastErrorMessage();
      } else {
        sockaddr_in address;
        address.sin_family = AF_INET;

        try {
          address.sin_port = htons(port);
          address.sin_addr.s_addr = htonl( addr.getValue());
        } catch(...) {/* do nothing */}

        int bres = -1;
        try{bres=bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof address);}catch(...){bres=-1;}

        int lres = -1;
        if (bres == 0) {
          try{lres=listen(listener, SOMAXCONN);}catch(...){lres=-1;}
        }

        if (bres != 0) {
          message = "bind failed, " + lastErrorMessage();
        } else if (lres != 0) {
            message = "listen failed, " + lastErrorMessage();
        } else {
          epoll_event listenEvent;
          listenEvent.events = EPOLLONESHOT;
          listenEvent.data.ptr = nullptr;

          int pres = -1;
          try{pres=epoll_ctl(dispatcher.getEpoll(), EPOLL_CTL_ADD,
            listener, &listenEvent);}catch(...){pres=-1;}

          if (pres == -1) {
            message = "epoll_ctl failed, " + lastErrorMessage();
          } else {
            context = nullptr;
            return;
          }
        }
      }
    }

    int result = -1;
    try{result=close(listener);}catch(...){result=-1;}
    if (result) {}
    assert(result != -1);
  }

  throw std::runtime_error("TcpListener::TcpListener, " + message);
}

TcpListener::TcpListener(TcpListener&& other) : dispatcher(other.dispatcher) {
  if (other.dispatcher != nullptr) {
    assert(other.context == nullptr);
    listener = other.listener;
    context = nullptr;
    other.dispatcher = nullptr;
  }
}

TcpListener::~TcpListener() {
  if (dispatcher != nullptr) {
    assert(context == nullptr);
    int result = -1;
    try{result=close(listener);}catch(...){result=-1;}
    if (result) {}
    assert(result != -1);
  }
}

TcpListener& TcpListener::operator=(TcpListener&& other) {
  if (dispatcher != nullptr) {
    assert(context == nullptr);

    int cres = -1;
    try{cres=close(listener);}catch(...){cres=-1;}
    if (cres == -1) {
      throw std::runtime_error("TcpListener::operator=, close failed, " + lastErrorMessage());
    }
  }

  dispatcher = other.dispatcher;
  if (other.dispatcher != nullptr) {
    assert(other.context == nullptr);
    listener = other.listener;
    context = nullptr;
    other.dispatcher = nullptr;
  }

  return *this;
}

TcpConnection TcpListener::accept() {
  assert(dispatcher != nullptr);
  assert(context == nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  ContextPair contextPair;
  OperationContext listenerContext;
  listenerContext.interrupted = false;
  listenerContext.context = dispatcher->getCurrentContext();

  contextPair.writeContext = nullptr;
  contextPair.readContext = &listenerContext;

  epoll_event listenEvent;
  listenEvent.events = EPOLLIN | EPOLLONESHOT;
  listenEvent.data.ptr = &contextPair;
  std::string message;

  int eres = -1;
  try{eres=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD, listener,
    &listenEvent);}catch(...){eres=-1;}

  if (eres == -1) {
    message = "epoll_ctl failed, " + lastErrorMessage();
  } else {
    context = &listenerContext;
    dispatcher->getCurrentContext()->interruptProcedure = [&]() {
        assert(dispatcher != nullptr);
        assert(context != nullptr);
        OperationContext* listenerContext = static_cast<OperationContext*>(context);
        if (!listenerContext->interrupted) {
          epoll_event listenEvent;
          listenEvent.events = EPOLLONESHOT;
          listenEvent.data.ptr = nullptr;

          int eres = -1;
          try{eres=epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_MOD, listener,
            &listenEvent);}catch(...){eres=-1;}

          if (eres == -1) {
            throw std::runtime_error("TcpListener::accept, interrupt procedure, epoll_ctl failed, " + lastErrorMessage() );
          }

          listenerContext->interrupted = true;
          dispatcher->pushContext(listenerContext->context);
        }
    };

    dispatcher->dispatch();
    dispatcher->getCurrentContext()->interruptProcedure = nullptr;
    assert(dispatcher != nullptr);
    assert(listenerContext.context == dispatcher->getCurrentContext());
    assert(contextPair.writeContext == nullptr);
    assert(context == &listenerContext);
    context = nullptr;
    listenerContext.context = nullptr;
    if (listenerContext.interrupted) {
      throw InterruptedException();
    }

    if((listenerContext.events & (EPOLLERR | EPOLLHUP)) != 0) {
      throw std::runtime_error("TcpListener::accept, accepting failed");
    }

    sockaddr inAddr;
    socklen_t inLen = sizeof(inAddr);
    int connection = -1;
    try{connection=::accept(listener, &inAddr, &inLen);}catch(...){connection=-1;}
    if (connection == -1) {
      message = "accept failed, " + lastErrorMessage();
    } else {

      int flags=-1;
      try{flags=fcntl(connection, F_GETFL, 0);}catch(...){flags=-1;}
      int flags2=-1;
      if (flags != -1){try{flags2=fcntl(connection, F_SETFL,
        flags | O_NONBLOCK);}catch(...){flags2=-1;}}

      if (flags == -1 || flags2 == -1) {
        message = "fcntl failed, " + lastErrorMessage();
      } else {
        return TcpConnection(*dispatcher, connection);
      }

      int result = -1;
      try{result=close(connection);}catch(...){result=-1;}
      if (result) {}
      assert(result != -1);
    }
  }

  throw std::runtime_error("TcpListener::accept, " + message);
}

}
