// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "TcpConnection.h"
#include <cassert>

#include <netinet/in.h>
#include <sys/event.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>

#include "Dispatcher.h"
#include <System/ErrorMessage.h>
#include <System/InterruptedException.h>
#include <System/IpAddress.h>

namespace System {

TcpConnection::TcpConnection() : dispatcher(nullptr) {
}

TcpConnection::TcpConnection(TcpConnection&& other) : dispatcher(other.dispatcher) {
  if (other.dispatcher != nullptr) {
    assert(other.readContext == nullptr);
    assert(other.writeContext == nullptr);
    connection = other.connection;
    readContext = nullptr;
    writeContext = nullptr;
    other.dispatcher = nullptr;
  }
}

TcpConnection::~TcpConnection() {
  if (dispatcher != nullptr) {
    assert(readContext == nullptr);
    assert(writeContext == nullptr);
    int result = -1;
    try{result=close(connection);}catch(...){result=-1;}
    assert(result != -1);
  }
}

TcpConnection& TcpConnection::operator=(TcpConnection&& other) {
  if (dispatcher != nullptr) {
    assert(readContext == nullptr);
    assert(writeContext == nullptr);
    int cresult=-1;
    try{cresult=close(connection);}catch(...){cresult=-1;}
    if (cresult == -1) {
      //throw std::runtime_error("TcpConnection::operator=, close failed, " + lastErrorMessage());
      std::cerr << "TcpConnection::operator=, close failed, " << lastErrorMessage() << std::endl;
    }
  }

  dispatcher = other.dispatcher;
  if (other.dispatcher != nullptr) {
    assert(other.readContext == nullptr);
    assert(other.writeContext == nullptr);
    connection = other.connection;
    readContext = nullptr;
    writeContext = nullptr;
    other.dispatcher = nullptr;
  }

  return *this;
}

size_t TcpConnection::read(uint8_t* data, size_t size, Logging::LoggerRef &logger, bool bSynchronous) {
  assert(dispatcher != nullptr);
  assert(readContext == nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  std::string message;
  ssize_t transferred = -1;
  try{transferred = ::recv(connection, (void *)data, size, 0);}catch(...){transferred=-1;}
  if (transferred == -1) {
    if (errno != EAGAIN  && errno != EWOULDBLOCK) {
      message = "recv failed, " + lastErrorMessage();
    } else {
      OperationContext context;
      context.context = dispatcher->getCurrentContext();
      context.interrupted = false;
      struct kevent event;
      int kresult = 0;
      try{EV_SET(&event, connection, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT, 0, 0, &context);}
        catch(...){kresult=-1;}
      if (kresult != -1) {
        kresult=-1;
        try{kresult=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){kresult=-1;}
      }
      if (kresult == -1) {
        message = "kevent failed, " + lastErrorMessage();
      } else {
        readContext = &context;
        dispatcher->getCurrentContext()->interruptProcedure = [&] {
          assert(dispatcher != nullptr);
          assert(readContext != nullptr);
          OperationContext* context = static_cast<OperationContext*>(readContext);
          if (!context->interrupted) {
            struct kevent event;
            int kresult = 0;
            try{EV_SET(&event, connection, EVFILT_READ, EV_DELETE | EV_DISABLE, 0, 0, NULL);}
              catch(...){kresult=-1;}
            if (kresult != -1) {
              kresult=-1;
              try{kresult = kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){kresult=-1;} 
            }
            if (kresult == -1) {
              //throw std::runtime_error("TcpListener::interruptionProcedure, kevent failed, " + lastErrorMessage());
              //std::cerr << "TcpListener::interruptionProcedure, kevent failed, " << lastErrorMessage() << std::endl;
            }
            
            context->interrupted = true;
            dispatcher->pushContext(context->context);
          }
        };
        
        dispatcher->dispatch();
        dispatcher->getCurrentContext()->interruptProcedure = nullptr;
        assert(dispatcher != nullptr);
        assert(context.context == dispatcher->getCurrentContext());
        assert(readContext == &context);
        readContext = nullptr;
        context.context = nullptr;
        if (context.interrupted) {
          throw InterruptedException();
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

    /* do nothing */
    //throw std::runtime_error("TcpConnection::read, " + message);
    //std::cerr << "runtime error TcpConnection::read, " << message << std::endl;
  }

  assert(transferred <= static_cast<ssize_t>(size));
  return transferred;
}

size_t TcpConnection::write(const uint8_t* data, size_t size, Logging::LoggerRef &logger) {
  assert(dispatcher != nullptr);
  assert(writeContext == nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  std::string message;
  if (size == 0) {
    int sresult=-1;
    try{sresult=shutdown(connection, SHUT_WR);}catch(...){sresult=-1;}
    if (sresult == -1) {
      /* do nothing */
      //throw std::runtime_error("TcpConnection::write, shutdown failed, " + lastErrorMessage());
      //std::cerr << "TcpConnection::write, shutdown failed, " << lastErrorMessage() << std::endl;
    }

    return 0;
  }

  ssize_t transferred = -1;
  try{transferred=::send(connection, (void *)data, size, 0);}catch(...){transferred=-1;}
  if (transferred == -1) {
    if (errno != EAGAIN  && errno != EWOULDBLOCK) {
      message = "send failed, " + lastErrorMessage();
    } else {
      OperationContext context;
      context.context = dispatcher->getCurrentContext();
      context.interrupted = false;
      struct kevent event;
      int kresult=0;
      try{EV_SET(&event, connection, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, &context);}
        catch(...){kresult=-1;}
      if (kresult != -1) {
        kresult=-1;
        try{kresult=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){kresult=-1;}
      }
      if (kresult == -1) {
        message = "kevent failed, " + lastErrorMessage();
      } else {
        writeContext = &context;
        dispatcher->getCurrentContext()->interruptProcedure = [&] {
          assert(dispatcher != nullptr);
          assert(writeContext != nullptr);
          OperationContext* context = static_cast<OperationContext*>(writeContext);
          if (!context->interrupted) {
            struct kevent event;
            int kresult = 0;
            try{EV_SET(&event, connection, EVFILT_WRITE, EV_DELETE | EV_DISABLE, 0, 0, NULL);}
              catch(...){kresult=-1;}
            if (kresult != -1) {
              kresult = -1; 
              try{kresult=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){kresult=-1;} 
            }
            if (kresult == -1) {
              //throw std::runtime_error("TcpListener::stop, kevent failed, " + lastErrorMessage());
              std::cerr << "TcpListener::stop, kevent failed, " << lastErrorMessage() << std::endl;
            }
            
            context->interrupted = true;
            dispatcher->pushContext(context->context);            
          }
        };
        
        dispatcher->dispatch();
        dispatcher->getCurrentContext()->interruptProcedure = nullptr;
        assert(dispatcher != nullptr);
        assert(context.context == dispatcher->getCurrentContext());
        assert(writeContext == &context);
        writeContext = nullptr;
        context.context = nullptr;
        if (context.interrupted) {
          throw InterruptedException();
        }

        ssize_t transferred = -1;
        try{transferred=::send(connection, (void *)data, size, 0);}catch(...){transferred=-1;}
        if (transferred == -1) {
          message = "send failed, " + lastErrorMessage();
        } else {
          assert(transferred <= static_cast<ssize_t>(size));
          return transferred;
        }
      }
    }

    // don't print anything out here... bad handles happen...
    //throw std::runtime_error("TcpConnection::write, " + message);
    //std::cerr << "TcpConnection::write, " << message << std::endl;
  }

  assert(transferred <= static_cast<ssize_t>(size));
  return transferred;
}

std::pair<IpAddress, uint16_t> TcpConnection::getPeerAddressAndPort() const {
  sockaddr_in addr;
  socklen_t size = sizeof(addr);
  int peerresult=-1;
  try {peerresult = getpeername(connection, reinterpret_cast<sockaddr*>(&addr), &size);}catch(...){peerresult=-1;}
  if (peerresult != 0) {
    //throw std::runtime_error("TcpConnection::getPeerAddress, getpeername failed, " + lastErrorMessage());
    std::cerr << "TcpConnection::getPeerAddress, getpeername failed, " << lastErrorMessage() << std::endl;
  }

  assert(size == sizeof(sockaddr_in));
  return std::make_pair(IpAddress(htonl(addr.sin_addr.s_addr)), htons(addr.sin_port));
}

TcpConnection::TcpConnection(Dispatcher& dispatcher, int socket) : dispatcher(&dispatcher), connection(socket), readContext(nullptr), writeContext(nullptr) {
  int val = 1;
  int optresult=-1;

  try{optresult=setsockopt(connection, SOL_SOCKET, SO_NOSIGPIPE,
    (void*)&val, sizeof val);}catch(...){optresult=-1;}

  if (optresult == -1) {
    // do nothing
    //throw std::runtime_error("TcpConnection::TcpConnection, setsockopt failed, " + lastErrorMessage());
    //std::cerr << "TcpConnection::TcpConnection, setsockopt failed, " << lastErrorMessage() << std::endl;
  }
}

}
