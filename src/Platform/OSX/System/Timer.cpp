// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "Timer.h"
#include <cassert>
#include <stdexcept>
#include <string>
#include <iostream>

#include <sys/errno.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#include "Dispatcher.h"
#include <System/ErrorMessage.h>
#include <System/InterruptedException.h>

namespace System {

Timer::Timer() : dispatcher(nullptr) {
}

Timer::Timer(Dispatcher& dispatcher) : dispatcher(&dispatcher), timer(-1), context(nullptr) {
}

Timer::Timer(Timer&& other) : dispatcher(other.dispatcher) {
  if (other.dispatcher != nullptr) {
    assert(other.context == nullptr);
    timer = other.timer;
    context = nullptr;
    other.dispatcher = nullptr;
  }
}

Timer::~Timer() {
  assert(dispatcher == nullptr || context == nullptr);
}

Timer& Timer::operator=(Timer&& other) {
  assert(dispatcher == nullptr || context == nullptr);
  dispatcher = other.dispatcher;
  if (other.dispatcher != nullptr) {
    assert(other.context == nullptr);
    timer = other.timer;
    context = nullptr;
    other.dispatcher = nullptr;
    other.timer = -1;
  }

  return *this;
}

void Timer::sleep(std::chrono::nanoseconds duration) {
  assert(dispatcher != nullptr);
  assert(context == nullptr);
  if (dispatcher->interrupted()) {
    throw InterruptedException();
  }

  OperationContext timerContext;
  timerContext.context = dispatcher->getCurrentContext();
  timerContext.interrupted = false;
  timer = dispatcher->getTimer();

  struct kevent event;
  EV_SET(&event, timer, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_NSECONDS, duration.count(), &timerContext);

  int kresult=-1;
  try{kresult=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){kresult=-1;}
  if (kresult == -1) {
    // do nothing
    std::cerr << "kevent 1 failed, errno: " << errno << " : " << lastErrorMessage() << std::endl;
    //throw std::runtime_error("Timer::stop, kevent1 failed, " + lastErrorMessage());
    return;
  }

  context = &timerContext;
  dispatcher->getCurrentContext()->interruptProcedure = [&] {
    assert(dispatcher != nullptr);
    assert(context != nullptr);
    OperationContext* timerContext = static_cast<OperationContext*>(context);
    if (!timerContext->interrupted) {
      struct kevent event;
      EV_SET(&event, timer, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);

      int kresult=-1;
      try{kresult=kevent(dispatcher->getKqueue(), &event, 1, NULL, 0, NULL);}catch(...){kresult=-1;}
      if (kresult == -1) {
        // do nothing
        //std::cerr << "kevent 2 failed, errno: " << errno << " : " << lastErrorMessage() << std::endl;
        //throw std::runtime_error("Timer::stop, kevent2 failed, " + lastErrorMessage());
      }

      dispatcher->pushContext(timerContext->context);
      timerContext->interrupted = true;
    }
  };
  
  dispatcher->dispatch();
  dispatcher->getCurrentContext()->interruptProcedure = nullptr;
  assert(dispatcher != nullptr);
  assert(timerContext.context == dispatcher->getCurrentContext());
  assert(context == &timerContext);
  context = nullptr;
  timerContext.context = nullptr;
  dispatcher->pushTimer(timer);
  if (timerContext.interrupted) {
    throw InterruptedException();
  }
}

}
