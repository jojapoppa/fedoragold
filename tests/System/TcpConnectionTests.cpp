// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define NOMINMAX

#include <System/Dispatcher.h>
#include <System/ContextGroup.h>
#include <System/Event.h>
#include <System/InterruptedException.h>
#include <System/IpAddress.h>
#include <System/TcpConnection.h>
#include <System/TcpConnector.h>
#include <System/TcpListener.h>
#include <System/TcpStream.h>
#include <System/Timer.h>
#include <gtest/gtest.h>

#include "Logging/LoggerManager.h"
using namespace Logging;
static LoggerManager tcpManager;
static LoggerRef tcplogger(tcpManager, "tcp tests");

using namespace System;

namespace {

const IpAddress LISTEN_ADDRESS("127.0.0.1");
const uint16_t LISTEN_PORT = 6666;

void fillRandomBuf(std::vector<uint8_t>& buf) {
  for (size_t i = 0; i < buf.size(); ++i) {
    buf[i] = static_cast<uint8_t>(rand() & 0xff);
  }
}

void fillRandomString(std::string& buf) {
  for (size_t i = 0; i < buf.size(); ++i) {
    buf[i] = static_cast<uint8_t>(rand() & 0xff);
  }
}

//std::string removePort(const std::string& address) {
//  size_t colonPosition = address.rfind(':');
//  if (colonPosition == std::string::npos) {
//    throw std::runtime_error("removePort");
//  }
//
//  return address.substr(0, colonPosition);
//}

}

class TcpConnectionTests : public testing::Test {
public:
  TcpConnectionTests() : listener(dispatcher, LISTEN_ADDRESS, LISTEN_PORT), contextGroup(dispatcher) {
  }

  void connect() {
    connection1 = TcpConnector(dispatcher).connect(LISTEN_ADDRESS, LISTEN_PORT);
    connection2 = listener.accept();
  }

protected:
  Dispatcher dispatcher;
  TcpListener listener;
  TcpConnection connection1;
  TcpConnection connection2;
  ContextGroup contextGroup;
};

TEST_F(TcpConnectionTests, sendAndClose) {
  connect();
  ASSERT_EQ(LISTEN_ADDRESS, connection1.getPeerAddressAndPort().first);
  ASSERT_EQ(LISTEN_ADDRESS, connection2.getPeerAddressAndPort().first);
  connection1.write(reinterpret_cast<const uint8_t*>("Test"), 4, tcplogger);
  uint8_t data[1024];
  size_t size = connection2.read(data, 1024, tcplogger);
  ASSERT_EQ(4, size);
  ASSERT_EQ(0, memcmp(data, "Test", 4));
  connection1 = TcpConnection();
  size = connection2.read(data, 1024, tcplogger);
  ASSERT_EQ(0, size);
}

TEST_F(TcpConnectionTests, stoppedState) {
  connect();
  bool stopped = false;
  contextGroup.spawn([&] {
    try {
      uint8_t data[1024];
      connection1.read(data, 1024, tcplogger);
    } catch (InterruptedException&) {
      stopped = true;
    }

    ASSERT_TRUE(stopped);
    contextGroup.interrupt();
    stopped = false;
    try {
      connection1.write(reinterpret_cast<const uint8_t*>("Test"), 4, tcplogger);
    } catch (InterruptedException&) {
      stopped = true;
    }
  });
  contextGroup.interrupt();
  contextGroup.wait();

  ASSERT_TRUE(stopped);
}

TEST_F(TcpConnectionTests, interruptRead) {
  connect();
  contextGroup.spawn([&]() {
    Timer(dispatcher).sleep(std::chrono::milliseconds(10));
    contextGroup.interrupt();
  });

  bool stopped = false;
  contextGroup.spawn([&]() {
    try {
      uint8_t data[1024];
      connection1.read(data, 1024, tcplogger);
    } catch (InterruptedException &) {
      stopped = true;
    }
  });

  contextGroup.wait();
  ASSERT_TRUE(stopped);
}

TEST_F(TcpConnectionTests, reuseWriteAfterInterrupt) {
  connect();
  contextGroup.spawn([&]() {
    Timer(dispatcher).sleep(std::chrono::milliseconds(10));
    contextGroup.interrupt();
  });

  bool stopped = false;
  contextGroup.spawn([&]() {
    try {
      uint8_t data[1024];
      connection1.read(data, 1024, tcplogger);
    } catch (InterruptedException &) {
      stopped = true;
    }
  });

  contextGroup.wait();
  ASSERT_TRUE(stopped);
  stopped = false;

  contextGroup.spawn([&]() {
    Timer(dispatcher).sleep(std::chrono::milliseconds(10));
    contextGroup.interrupt();
  });

  contextGroup.spawn([&] {
    try {
      uint8_t buff[1024];
      std::fill(std::begin(buff), std::end(buff), 0xff);
      connection1.write(buff, sizeof(buff), tcplogger); // write smth
      connection1 = TcpConnection();         // close connection
    } catch (InterruptedException&) {
      stopped = true;
    }
  });

  contextGroup.spawn([&]() {
    try {
      uint8_t data[1024];
      connection2.read(data, 1024, tcplogger);
    } catch (InterruptedException &) {
      stopped = true;
    }
  });

  contextGroup.wait();
  ASSERT_TRUE(!stopped);
}

TEST_F(TcpConnectionTests, reuseReadAfterInterrupt) {
  connect();
  contextGroup.spawn([&]() {
    Timer(dispatcher).sleep(std::chrono::milliseconds(10));
    contextGroup.interrupt();
  });

  bool stopped = false;
  contextGroup.spawn([&]() {
    try {
      uint8_t data[1024];
      connection1.read(data, 1024, tcplogger);
    } catch (InterruptedException &) {
      stopped = true;
    }
  });

  contextGroup.wait();
  ASSERT_TRUE(stopped);
  stopped = false;

  contextGroup.spawn([&]() {
    Timer(dispatcher).sleep(std::chrono::milliseconds(10));
    contextGroup.interrupt();
  });

  contextGroup.spawn([&] {
    try {
      uint8_t buff[1024];
      std::fill(std::begin(buff), std::end(buff), 0xff);
      connection2.write(buff, sizeof(buff), tcplogger); // write smth
      connection2 = TcpConnection();         // close connection
    } catch (InterruptedException&) {
      stopped = true;
    }
  });

  contextGroup.spawn([&]() {
    try {
      uint8_t data[1024];
      connection1.read(data, 1024, tcplogger);
    } catch (InterruptedException &) {
      stopped = true;
    }
  });

  contextGroup.wait();
  ASSERT_TRUE(!stopped);
}

TEST_F(TcpConnectionTests, sendBigChunk) {
  connect();
  
  const size_t bufsize =  15* 1024 * 1024; // 15MB
  std::vector<uint8_t> buf;
  buf.resize(bufsize);
  fillRandomBuf(buf);

  std::vector<uint8_t> incoming;
  Event readComplete(dispatcher);

  contextGroup.spawn([&]{
    uint8_t readBuf[1024];
    size_t readSize;
    while ((readSize = connection2.read(readBuf, sizeof(readBuf), tcplogger)) > 0) {
      incoming.insert(incoming.end(), readBuf, readBuf + readSize);
    }

    readComplete.set();
  });

  contextGroup.spawn([&]{
    uint8_t* bufPtr = &buf[0];
    size_t left = bufsize;
    while(left > 0) {
      auto transferred =  connection1.write(bufPtr, std::min(left, size_t(666)), tcplogger);
      left -= transferred;
      bufPtr += transferred;
    }

    connection1 = TcpConnection(); // close connection
  });

  readComplete.wait();

  ASSERT_EQ(bufsize, incoming.size());
  ASSERT_EQ(buf, incoming);
}

TEST_F(TcpConnectionTests, writeWhenReadWaiting) {
  connect();

  Event readStarted(dispatcher);
  Event readCompleted(dispatcher);
  Event writeCompleted(dispatcher);

  size_t writeSize = 0;
  bool readStopped = false;

  contextGroup.spawn([&]{
    try {
      uint8_t readBuf[1024];
      size_t readSize;
      readStarted.set();
      while ((readSize = connection2.read(readBuf, sizeof(readBuf), tcplogger)) > 0) {
      }
    } catch (InterruptedException&) {
      readStopped = true;
    }
    connection2 = TcpConnection();
    readCompleted.set();
  });

  readStarted.wait();

  contextGroup.spawn([&]{
    uint8_t writeBuf[1024];
    for (int i = 0; i < 100; ++i) {
      writeSize += connection2.write(writeBuf, sizeof(writeBuf), tcplogger);
    }
    //connection2.stop();
    contextGroup.interrupt();
    writeCompleted.set();
  });

  uint8_t readBuf[100];
  size_t readSize;
  size_t totalRead = 0;
  while ((readSize = connection1.read(readBuf, sizeof(readBuf), tcplogger)) > 0) {
    totalRead += readSize;
  }

  ASSERT_EQ(writeSize, totalRead);
  readCompleted.wait();
  ASSERT_TRUE(readStopped);
  writeCompleted.wait();
}

TEST_F(TcpConnectionTests, sendBigChunkThruTcpStream) {
  connect();
  const size_t bufsize = 15 * 1024 * 1024; // 15MB
  std::string buf;
  buf.resize(bufsize);
  fillRandomString(buf);

  std::string incoming;
  Event readComplete(dispatcher);

  contextGroup.spawn([&]{
    uint8_t readBuf[1024];
    size_t readSize;
    while ((readSize = connection2.read(readBuf, sizeof(readBuf), tcplogger)) > 0) {
      incoming.insert(incoming.end(), readBuf, readBuf + readSize);
    }

    readComplete.set();
  });


  contextGroup.spawn([&]{
    TcpStreambuf streambuf(connection1, tcplogger);
    std::iostream stream(&streambuf);

    stream << buf;
    stream.flush();

    connection1 = TcpConnection(); // close connection
  });

  readComplete.wait();

  ASSERT_EQ(bufsize, incoming.size());

  //ASSERT_EQ(buf, incoming); 
  for (size_t i = 0; i < bufsize; ++i) {
    ASSERT_EQ(buf[i], incoming[i]); //for better output.
  }
}
