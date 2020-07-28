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

#include "Dispatcher.h"
#include <cassert>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "ErrorMessage.h"

namespace System {

namespace {

struct ContextMakingData {
  Dispatcher* dispatcher;
  void* ucontext;
};

class MutextGuard {
public:
  MutextGuard(pthread_mutex_t& _mutex) : mutex(_mutex) {
    auto ret = pthread_mutex_lock(&mutex);
    if (ret != 0) {
      throw std::runtime_error("pthread_mutex_lock failed, " + errorMessage(ret));
    }
  }

  ~MutextGuard() {
    pthread_mutex_unlock(&mutex);
  }

private:
  pthread_mutex_t& mutex;
};

/*
static_assert(sizeof(pthread_mutex_t) != 1, "1");
static_assert(sizeof(pthread_mutex_t) != 2, "2");
static_assert(sizeof(pthread_mutex_t) != 3, "3");
static_assert(sizeof(pthread_mutex_t) != 4, "4");
static_assert(sizeof(pthread_mutex_t) != 5, "5");
static_assert(sizeof(pthread_mutex_t) != 6, "6");
static_assert(sizeof(pthread_mutex_t) != 7, "7");
static_assert(sizeof(pthread_mutex_t) != 8, "8");
static_assert(sizeof(pthread_mutex_t) != 9, "9");
static_assert(sizeof(pthread_mutex_t) != 10, "10");
static_assert(sizeof(pthread_mutex_t) != 11, "11");
static_assert(sizeof(pthread_mutex_t) != 12, "12");
static_assert(sizeof(pthread_mutex_t) != 13, "13");
static_assert(sizeof(pthread_mutex_t) != 14, "14");
static_assert(sizeof(pthread_mutex_t) != 15, "15");
static_assert(sizeof(pthread_mutex_t) != 16, "16");
static_assert(sizeof(pthread_mutex_t) != 17, "17");
static_assert(sizeof(pthread_mutex_t) != 18, "18");
static_assert(sizeof(pthread_mutex_t) != 19, "19");
static_assert(sizeof(pthread_mutex_t) != 20, "20");
static_assert(sizeof(pthread_mutex_t) != 21, "21");
static_assert(sizeof(pthread_mutex_t) != 22, "22");
static_assert(sizeof(pthread_mutex_t) != 23, "23");
static_assert(sizeof(pthread_mutex_t) != 24, "24");
static_assert(sizeof(pthread_mutex_t) != 25, "25");
static_assert(sizeof(pthread_mutex_t) != 26, "26");
static_assert(sizeof(pthread_mutex_t) != 27, "27");
static_assert(sizeof(pthread_mutex_t) != 28, "28");
static_assert(sizeof(pthread_mutex_t) != 29, "29");
static_assert(sizeof(pthread_mutex_t) != 30, "30");
static_assert(sizeof(pthread_mutex_t) != 31, "31");
static_assert(sizeof(pthread_mutex_t) != 32, "32");
static_assert(sizeof(pthread_mutex_t) != 33, "33");
static_assert(sizeof(pthread_mutex_t) != 34, "34");
static_assert(sizeof(pthread_mutex_t) != 35, "35");
static_assert(sizeof(pthread_mutex_t) != 36, "36");
static_assert(sizeof(pthread_mutex_t) != 37, "37");
static_assert(sizeof(pthread_mutex_t) != 38, "38");
static_assert(sizeof(pthread_mutex_t) != 39, "39");
static_assert(sizeof(pthread_mutex_t) != 40, "40");
static_assert(sizeof(pthread_mutex_t) != 41, "41");
static_assert(sizeof(pthread_mutex_t) != 42, "42");
static_assert(sizeof(pthread_mutex_t) != 43, "43");
static_assert(sizeof(pthread_mutex_t) != 44, "44");
static_assert(sizeof(pthread_mutex_t) != 45, "45");
static_assert(sizeof(pthread_mutex_t) != 46, "46");
static_assert(sizeof(pthread_mutex_t) != 47, "47");
static_assert(sizeof(pthread_mutex_t) != 48, "48");
static_assert(sizeof(pthread_mutex_t) != 49, "49");
static_assert(sizeof(pthread_mutex_t) != 50, "50");
static_assert(sizeof(pthread_mutex_t) != 51, "51");
static_assert(sizeof(pthread_mutex_t) != 52, "52");
static_assert(sizeof(pthread_mutex_t) != 53, "53");
static_assert(sizeof(pthread_mutex_t) != 54, "54");
static_assert(sizeof(pthread_mutex_t) != 55, "55");
static_assert(sizeof(pthread_mutex_t) != 56, "56");
static_assert(sizeof(pthread_mutex_t) != 57, "57");
static_assert(sizeof(pthread_mutex_t) != 58, "58");
static_assert(sizeof(pthread_mutex_t) != 59, "59");
static_assert(sizeof(pthread_mutex_t) != 60, "60");
static_assert(sizeof(pthread_mutex_t) != 61, "61");
static_assert(sizeof(pthread_mutex_t) != 62, "62");
static_assert(sizeof(pthread_mutex_t) != 63, "63");
static_assert(sizeof(pthread_mutex_t) != 64, "64");
*/

static_assert(Dispatcher::SIZEOF_PTHREAD_MUTEX_T == sizeof(pthread_mutex_t), "invalid pthread mutex size");

template<size_t A, size_t B> struct TAssertEquality {
  static_assert(A == B, "invalid pthread mutex size");
  static constexpr bool _cResult = (A==B);
};

static constexpr bool _cIsEqual = TAssertEquality<Dispatcher::SIZEOF_PTHREAD_MUTEX_T, sizeof(pthread_mutex_t)>::_cResult;

const size_t STACK_SIZE = 64 * 1024;

};

Dispatcher::Dispatcher() {
  std::string message;
  epoll = Sys::epoll_create1(0);
  if (epoll == -1) {
    message = "epoll_create1 failed, " + lastErrorMessage();
  } else {
    mainContext.ucontext = new NativeContext;
    if (sys.getcontext(reinterpret_cast<NativeContext *>(mainContext.ucontext)) == -1) {
      message = "getcontext failed, " + lastErrorMessage();
    } else {
      remoteSpawnEvent = sys.eventfd(0, O_NONBLOCK);
      if(remoteSpawnEvent == -1) {
        message = "eventfd failed, " + lastErrorMessage();
      } else {
        remoteSpawnEventContext.writeContext = nullptr;
        remoteSpawnEventContext.readContext = nullptr;

        epoll_event remoteSpawnEventEpollEvent;
        remoteSpawnEventEpollEvent.events = EPOLLIN;
        remoteSpawnEventEpollEvent.data.ptr = &remoteSpawnEventContext;

        if (Sys::epoll_ctl(epoll, EPOLL_CTL_ADD, remoteSpawnEvent, &remoteSpawnEventEpollEvent) == -1) {
          message = "epoll_ctl failed, " + lastErrorMessage();
        } else {
          *reinterpret_cast<pthread_mutex_t*>(this->mutex) = pthread_mutex_t(PTHREAD_MUTEX_INITIALIZER);

          mainContext.interrupted = false;
          mainContext.group = &contextGroup;
          mainContext.groupPrev = nullptr;
          mainContext.groupNext = nullptr;
          mainContext.inExecutionQueue = false;
          contextGroup.firstContext = nullptr;
          contextGroup.lastContext = nullptr;
          contextGroup.firstWaiter = nullptr;
          contextGroup.lastWaiter = nullptr;
          currentContext = &mainContext;
          firstResumingContext = nullptr;
          firstReusableContext = nullptr;
          runningContextCount = 0;
          return;
        }

        auto result = close(remoteSpawnEvent);
        assert(result == 0);
      }
    }

    auto result = close(epoll);
    assert(result == 0);
  }

  throw std::runtime_error("Dispatcher::Dispatcher, "+message);
}

Dispatcher::~Dispatcher() {
  for (NativeContext* context = contextGroup.firstContext; context != nullptr; context = context->groupNext) {
    interrupt(context);
  }

  yield();
  assert(contextGroup.firstContext == nullptr);
  assert(contextGroup.firstWaiter == nullptr);
  assert(firstResumingContext == nullptr);
  assert(runningContextCount == 0);
  while (firstReusableContext != nullptr) {
    auto ucontext = firstReusableContext->ucontext;
    auto stackPtr = static_cast<uint8_t *>(firstReusableContext->stackPtr);
    firstReusableContext = firstReusableContext->next;
    delete[] stackPtr;
    sys.deletecontext(ucontext);
  }

  while (!timers.empty()) {
    int result = ::close(timers.top());
    assert(result == 0);
    timers.pop();
  }

  auto result = close(epoll);
  assert(result == 0);
  result = close(remoteSpawnEvent);
  assert(result == 0);
  result = pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(this->mutex));
  assert(result == 0);
}

void Dispatcher::clear() {
  while (firstReusableContext != nullptr) {
    auto ucontext = firstReusableContext->ucontext;
    auto stackPtr = static_cast<uint8_t *>(firstReusableContext->stackPtr);
    firstReusableContext = firstReusableContext->next;
    delete[] stackPtr;
    sys.deletecontext(ucontext);
  }

  while (!timers.empty()) {
    int result = ::close(timers.top());
    if (result == -1) {
      throw std::runtime_error("Dispatcher::clear, close failed, "  + lastErrorMessage());
    }

    timers.pop();
  }
}

void Dispatcher::dispatch() {
  NativeContext* context;
  for (;;) {
    if (firstResumingContext != nullptr) {
      context = firstResumingContext;
      firstResumingContext = context->next;

      //when compiling in debug i was hitting this assert...
      //assert(context->inExecutionQueue);
      context->inExecutionQueue = false;
      break;
    }

    epoll_event event;
    int count = Sys::epoll_wait(epoll, &event, 1, -1);
    if (count == 1) {
      ContextPair *contextPair = static_cast<ContextPair*>(event.data.ptr);
      if(((event.events & (EPOLLIN | EPOLLOUT)) != 0) && contextPair->readContext == nullptr && contextPair->writeContext == nullptr) {
        uint64_t buf;
        auto transferred = read(remoteSpawnEvent, &buf, sizeof buf);
        if(transferred == -1) {
            throw std::runtime_error("Dispatcher::dispatch, read(remoteSpawnEvent) failed, " + lastErrorMessage());
        }

        MutextGuard guard(*reinterpret_cast<pthread_mutex_t*>(this->mutex));
        while (!remoteSpawningProcedures.empty()) {
          spawn(std::move(remoteSpawningProcedures.front()));
          remoteSpawningProcedures.pop();
        }

        continue;
      }

      if ((event.events & EPOLLOUT) != 0) {
        context = contextPair->writeContext->context;
        contextPair->writeContext->events = event.events;
      } else if ((event.events & EPOLLIN) != 0) {
        context = contextPair->readContext->context;
        contextPair->readContext->events = event.events;
      } else {
        continue;
      }

      assert(context != nullptr);
      break;
    }

    if (errno != EINTR) {
      throw std::runtime_error("Dispatcher::dispatch, epoll_wait failed, "  + lastErrorMessage());
    }
  }

  if (context != currentContext) {
    void* oldContext = static_cast<void*>(currentContext->ucontext);
    currentContext = context;
    if (sys.swapcontext(oldContext, static_cast<void*>(context->ucontext)) == -1) {
      throw std::runtime_error("Dispatcher::dispatch, sys.swapcontext failed, " + lastErrorMessage());
    }
  }
}

NativeContext* Dispatcher::getCurrentContext() const {
  return currentContext;
}

void Dispatcher::interrupt() {
  interrupt(currentContext);
}

void Dispatcher::interrupt(NativeContext* context) {
  assert(context!=nullptr);
  if (!context->interrupted) {
    if (context->interruptProcedure != nullptr) {
      context->interruptProcedure();
      context->interruptProcedure = nullptr;
    } else {
      context->interrupted = true;
    }
  }
}

bool Dispatcher::interrupted() {
  if (currentContext->interrupted) {
    currentContext->interrupted = false;
    return true;
  }

  return false;
}

void Dispatcher::pushContext(NativeContext* context) {
  assert(context != nullptr);

  if (context->inExecutionQueue)
    return;

  context->next = nullptr;
  context->inExecutionQueue = true;

  if(firstResumingContext != nullptr) {
    assert(lastResumingContext != nullptr);
    lastResumingContext->next = context;
  } else {
    firstResumingContext = context;
  }

  lastResumingContext = context;
}

void Dispatcher::remoteSpawn(std::function<void()>&& procedure) {
  {
    MutextGuard guard(*reinterpret_cast<pthread_mutex_t*>(this->mutex));
    remoteSpawningProcedures.push(std::move(procedure));
  }
  uint64_t one = 1;
  auto transferred = write(remoteSpawnEvent, &one, sizeof one);
  if(transferred == - 1) {
    throw std::runtime_error("Dispatcher::remoteSpawn, write failed, " + lastErrorMessage());
  }
}

void Dispatcher::spawn(std::function<void()>&& procedure) {
  NativeContext* context = &getReusableContext();
  if(contextGroup.firstContext != nullptr) {
    context->groupPrev = contextGroup.lastContext;
    assert(contextGroup.lastContext->groupNext == nullptr);
    contextGroup.lastContext->groupNext = context;
  } else {
    context->groupPrev = nullptr;
    contextGroup.firstContext = context;
    contextGroup.firstWaiter = nullptr;
  }

  context->interrupted = false;
  context->group = &contextGroup;
  context->groupNext = nullptr;
  context->procedure = std::move(procedure);
  contextGroup.lastContext = context;
  pushContext(context);
}

void Dispatcher::yield() {
  for(;;){
    epoll_event events[16];
    int count = Sys::epoll_wait(epoll, events, 16, 0);
    if (count == 0) {
      break;
    }

    if(count > 0) {
      for(int i = 0; i < count; ++i) {
        ContextPair *contextPair = static_cast<ContextPair*>(events[i].data.ptr);
        if(((events[i].events & (EPOLLIN | EPOLLOUT)) != 0) && contextPair->readContext == nullptr && contextPair->writeContext == nullptr) {
          uint64_t buf;
          auto transferred = read(remoteSpawnEvent, &buf, sizeof buf);
          if(transferred == -1) {
            throw std::runtime_error("Dispatcher::dispatch, read(remoteSpawnEvent) failed, " + lastErrorMessage());
          }

          MutextGuard guard(*reinterpret_cast<pthread_mutex_t*>(this->mutex));
          while (!remoteSpawningProcedures.empty()) {
            spawn(std::move(remoteSpawningProcedures.front()));
            remoteSpawningProcedures.pop();
          }

          continue;
        }

        if ((events[i].events & EPOLLOUT) != 0) {
          if (contextPair->writeContext != nullptr) {
            if (contextPair->writeContext->context != nullptr) {
              contextPair->writeContext->context->interruptProcedure = nullptr;
            }
            pushContext(contextPair->writeContext->context);
            contextPair->writeContext->events = events[i].events;
          }
        } else if ((events[i].events & EPOLLIN) != 0) {
          if (contextPair->readContext != nullptr) {
            if (contextPair->readContext->context != nullptr) {
              contextPair->readContext->context->interruptProcedure = nullptr;
            }
            pushContext(contextPair->readContext->context);
            contextPair->readContext->events = events[i].events;
          }
        } else {
          continue;
        }
      }
    } else {
      if (errno != EINTR) {
        throw std::runtime_error("Dispatcher::dispatch, epoll_wait failed, " + lastErrorMessage());
      }
    }
  }

  if (firstResumingContext != nullptr) {
    pushContext(currentContext);
    dispatch();
  }
}

int Dispatcher::getEpoll() const {
  return epoll;
}

NativeContext& Dispatcher::getReusableContext() {
  if(firstReusableContext == nullptr) {
    NativeContext* newlyCreatedContext = new NativeContext;
    if (sys.getcontext(newlyCreatedContext) == -1) { //makecontext precondition
      throw std::runtime_error("Dispatcher::getReusableContext, getcontext failed, " + lastErrorMessage());
    }

    auto stackPointer = new uint8_t[STACK_SIZE];
    newlyCreatedContext->stackPtr = stackPointer;

    ContextMakingData makingContextData {this, (void *)newlyCreatedContext};
    sys.makecontext(newlyCreatedContext, STACK_SIZE, (void(*)())contextProcedureStatic, 1, reinterpret_cast<int*>(&makingContextData));

    void* oldContext = static_cast<void*>(currentContext->ucontext);
    if (sys.swapcontext(oldContext, newlyCreatedContext) == -1) {
      throw std::runtime_error("Dispatcher::getReusableContext, swapcontext failed, " + lastErrorMessage());
    }

    assert(firstReusableContext != nullptr);
    assert(firstReusableContext->ucontext == newlyCreatedContext);
    firstReusableContext->stackPtr = stackPointer;
  };

  NativeContext* context = firstReusableContext;
  firstReusableContext = firstReusableContext-> next;
  return *context;
}

void Dispatcher::pushReusableContext(NativeContext& context) {
  context.next = firstReusableContext;
  firstReusableContext = &context;
  --runningContextCount;
}

int Dispatcher::getTimer() {
  int timer;
  if (timers.empty()) {
    timer = Sys::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    epoll_event timerEvent;
    timerEvent.events = EPOLLONESHOT;
    timerEvent.data.ptr = nullptr;

    if (Sys::epoll_ctl(getEpoll(), EPOLL_CTL_ADD, timer, &timerEvent) == -1) {
      throw std::runtime_error("Dispatcher::getTimer, epoll_ctl failed, "  + lastErrorMessage());
    }
  } else {
    timer = timers.top();
    timers.pop();
  }

  return timer;
}

void Dispatcher::pushTimer(int timer) {
  timers.push(timer);
}

void Dispatcher::contextProcedure(void* ucontext) {
  assert(firstReusableContext == nullptr);
  NativeContext context;
  context.ucontext = ucontext;
  context.interrupted = false;
  context.next = nullptr;
  context.inExecutionQueue = false;
  firstReusableContext = &context;
  void* oldContext = static_cast<void*>(context.ucontext);
  if (sys.swapcontext(oldContext, static_cast<void*>(currentContext->ucontext)) == -1) {
    throw std::runtime_error("Dispatcher::contextProcedure, swapcontext failed, " + lastErrorMessage());
  }

  for (;;) {
    ++runningContextCount;
    try {
      context.procedure();
    } catch(...) {
    }

    if (context.group != nullptr) {
      if (context.groupPrev != nullptr) {
        assert(context.groupPrev->groupNext == &context);
        context.groupPrev->groupNext = context.groupNext;
        if (context.groupNext != nullptr) {
          assert(context.groupNext->groupPrev == &context);
          context.groupNext->groupPrev = context.groupPrev;
        } else {
          assert(context.group->lastContext == &context);
          context.group->lastContext = context.groupPrev;
        }
      } else {
        assert(context.group->firstContext == &context);
        context.group->firstContext = context.groupNext;
        if (context.groupNext != nullptr) {
          assert(context.groupNext->groupPrev == &context);
          context.groupNext->groupPrev = nullptr;
        } else {
          assert(context.group->lastContext == &context);
          if (context.group->firstWaiter != nullptr) {
            if (firstResumingContext != nullptr) {
              assert(lastResumingContext->next == nullptr);
              lastResumingContext->next = context.group->firstWaiter;
            } else {
              firstResumingContext = context.group->firstWaiter;
            }

            lastResumingContext = context.group->lastWaiter;
            context.group->firstWaiter = nullptr;
          }
        }
      }

      pushReusableContext(context);
    }

    dispatch();
  }
};

void Dispatcher::contextProcedureStatic(void *context) {
  ContextMakingData* makingContextData = reinterpret_cast<ContextMakingData*>(context);
  makingContextData->dispatcher->contextProcedure(makingContextData->ucontext);
}

}
