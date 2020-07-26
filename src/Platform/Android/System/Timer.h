// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <chrono>

#include <System/ErrorMessage.h>

namespace System {

class Dispatcher;

// For some reason these are not defined on arm properly.
// ... the values are either 11 or 35
#define EAGAIN 35
#define EWOULDBLOCK 35

class Timer {
public:
  Timer();
  explicit Timer(Dispatcher& dispatcher);
  Timer(const Timer&) = delete;
  Timer(Timer&& other);
  ~Timer();
  Timer& operator=(const Timer&) = delete;
  Timer& operator=(Timer&& other);
  void sleep(std::chrono::nanoseconds duration);

private:
  Dispatcher* dispatcher;
  void* context;
  int timer;

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
