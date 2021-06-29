// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "TcpListener.h"
#include <cassert>
#include <stdexcept>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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
  try{listener=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);}catch(...){/*nothing*/}
  if (listener == -1) {
    message = "socket failed, " + lastErrorMessage();
  } else {
    int flags = -1;
    try{flags=fcntl(listener, F_GETFL, 0);}catch(...){/*nothing*/}
    int flags2 = -1;
    try{flags2=fcntl(listener, F_SETFL, flags | O_NONBLOCK);}catch(...){/*nothing*/}
    if (flags == -1 || flags2 == -1) {
      message = "fcntl failed, " + lastErrorMessage();
    } else {
      int on = 1;
      int result=-1;
      try{result=setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);}catch(...){/*nothing*/}
      if (result == -1) {
        message = "setsockopt failed, " + lastErrorMessage();
      } else {
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(addr.getValue());
        int bresult=-1;
        try{bresult=bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof address);}catch(...){/*nothing*/}
        if (bresult != 0) {
          message = "bind failed, " + lastErrorMessage();
        } else if (listen(listener, SOMAXCONN) != 0) {
          message = "listen failed, " + lastErrorMessage();
        } else {
          struct kevent event;
          EV_SET(&event, listener, EVFILT_READ, EV_ADD | EV_DISABLE | EV_CLEAR, 0, SOMAXCONN, NULL);

          int kres=-1;
          try{kres=kevent(dispatcher.getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){/*nothing*/}
          if (kres == -1) {
            message = "kevent failed, " + lastErrorMessage();
          } else {
            context = nullptr;
            return;
          }
        }
      }
    }

    int cresult = -1;
    try{cresult=close(listener);}catch(...){/*nothing*/}
    if (cresult == -1) {
      message = "close failed, " + lastErrorMessage();
    }
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
    try{result=close(listener);}catch(...){/*nothing*/}
    assert(result != -1);
  }
}

TcpListener& TcpListener::operator=(TcpListener&& other) {
  if (dispatcher != nullptr) {
    assert(context == nullptr);
    int cresult=-1;
    try{cresult=close(listener);}catch(...){/*nothing*/}
    if (cresult == -1) {
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

  std::string message;
  OperationContext listenerContext;
  listenerContext.context = dispatcher->getCurrentContext();
  listenerContext.interrupted = false;
  struct kevent event;
  EV_SET(&event, listener, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR , 0, SOMAXCONN, &listenerContext);
  int kresult=-1;
  try{kresult=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){/*nothing*/}
  if (kresult == -1) {
    message = "kevent failed, " + lastErrorMessage();
  } else {
    context = &listenerContext;
    dispatcher->getCurrentContext()->interruptProcedure = [&] {
      assert(dispatcher != nullptr);
      assert(context != nullptr);
      OperationContext* listenerContext = static_cast<OperationContext*>(context);
      if (!listenerContext->interrupted) {
        
        struct kevent event;
        EV_SET(&event, listener, EVFILT_READ, EV_DELETE | EV_DISABLE, 0, 0, NULL);
    
        int kres=-1;
        try{kres=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){/*nothing*/}    
        if (kres == -1) {
          throw std::runtime_error("TcpListener::stop, kevent failed, " + lastErrorMessage());
        }
        
        listenerContext->interrupted = true;
        dispatcher->pushContext(listenerContext->context);
      }
    };
    
    dispatcher->dispatch();
    dispatcher->getCurrentContext()->interruptProcedure = nullptr;
    assert(dispatcher != nullptr);
    assert(listenerContext.context == dispatcher->getCurrentContext());
    assert(context == &listenerContext);
    context = nullptr;
    listenerContext.context = nullptr;
    if (listenerContext.interrupted) {
      throw InterruptedException();
    }

    sockaddr inAddr;
    socklen_t inLen = sizeof(inAddr);
    int connection = -1;
    try{connection=::accept(listener, &inAddr, &inLen);}catch(...){/*nothing*/}
    if (connection == -1) {
      message = "accept failed, " + lastErrorMessage();
    } else {
      int flags = -1;
      try{flags=fcntl(connection, F_GETFL, 0);}catch(...){/*nothing*/}
      int flags2 = -1;
      try{flags2=fcntl(connection, F_SETFL, flags | O_NONBLOCK);}catch(...){/*nothing*/}
      if (flags == -1 || flags2 == -1) {
        message = "fcntl failed, " + lastErrorMessage();
      } else {
        return TcpConnection(*dispatcher, connection);
      }
    }
  }

  throw std::runtime_error("TcpListener::accept, " + message);
}

}
