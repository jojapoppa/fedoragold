// Copyright (c) 2012-2014, The CryptoNote developers, The Bytecoin developers
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

#pragma once

/* The stack size defaults to 1MB. */
#ifndef FIBER_STACK_SIZE
#       define FIBER_STACK_SIZE (1024 * 1024)
#endif /* FIBER_STACK_SIZE */

#include <functional>
#include <queue>
#include <stack>

#ifndef bool_t
#       define bool_t unsigned char
#endif /* bool_t */

struct fiber_t;

/**
 * @brief Fiber processor, will callback to this.
 *
 * @param[in] fb - On which fiber is calling.
 */
typedef void (* fiber_proc)(struct fiber_t* /*fb*/);

class Sys {
public:
  Sys();
  Sys(const Sys&) = delete;
  ~Sys();
  Sys& operator=(const Sys&) = delete;

  //void* getIoService();

  int getcontext(void *ucp);
  int swapcontext(void *oucp, const void *ucp);
  void makecontext(void *context, size_t stacksize, void (*func)(), int argc, int *argv);
  void deletecontext(void *context);

  // there may be emulations that exist to implement this over websockets...
  // ... https://github.com/emscripten-core/emscripten/issues/5033
  static int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
  static int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
  static int epoll_create1(int flags);

  // this is the call that's used eventfd(0, O_NONBLOCK); (from Dispatcher.cpp)
  // (basically it uses a flag that tells you which system call to make)
  // ... https://github.com/emscripten-core/emscripten/issues/6708
  static int eventfd(unsigned int count, int flags);

  // https://github.com/emscripten-core/emscripten/issues/4414
  static int timerfd_create(int clockid, int flags);
  static int timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);

private:
  void* ioService;

  /**
   * @brief Checks whether a fiber is the current active one.
   *
   * @param[in] fb - Specific fiber to check with.
   * @return - True if `fb` is the current active fiber.
   */
  bool_t fiber_is_current(const fiber_t* const fb);
        //if (!fb) return false;
        //return *fb->current == fb;

  /**
   * @brief Switches to a specific fiber. The `fiber_proc proc` processer will be called.
   *
   * @param[in] fb - The target fiber to switch to.
   * @return - True for succeed. This function returns when some fiber switches back here.
   */
  bool_t fiber_switch(fiber_t* fb);
        //fiber_t* curr = 0;
        //if (!fb) return false;
        //curr = *fb->current;
        //*fb->current = fb;
        //swapcontext((ucontext_t*)curr->context, (ucontext_t*)fb->context);
        //return true;

  /**
   * @brief Deletes a fiber. Should delete minor fibers before deleting the primary fiber.
   *
   * @param[in] fb - The fiber to be deleted.
   * @return - True for succeed.
   */
  bool_t fiber_delete(fiber_t* fb);
        //if (!fb) return false;
        //if (fb->proc) {
        //    ucontext_t* ctx = (ucontext_t*)fb->context;
        //    free(ctx->uc_stack.ss_sp);
        //} else {
        // free(fb->current);
        //}
        //free(fb->context);
        //free(fb);
        //return true;

  /**
   * @brief Creates a fiber. Should create the primary fiber before creating minor fibers.
   *
   * @param[in] primary - The original primary fiber, pass `NULL` if it's the primary fiber to be created.
   * @param[in] stack - Reserved stack size for this fiber, pass 0 if it's the primary fiber to be created,
   *                    or uses `FIBER_STACK_SIZE` for minor fibers.
   * @param[in] run - Fiber processor.
   * @param[in] userdata - Userdata of fiber.
   * @return - Created fiber.
   */
  fiber_t* fiber_create(fiber_t* primary, size_t stack, fiber_proc run, void* userdata);
        //fiber_t* ret = 0;
        //ucontext_t* ctx = 0;
        //if ((!primary && run) || (primary && !run)) return ret;
        //ret = (fiber_t*)malloc(sizeof(fiber_t));
        //ret->proc = run;
        //ret->userdata = userdata;
        //if (primary && run) {
        //    if (!stack) stack = FIBER_STACK_SIZE;
        //    ctx = (ucontext_t*)malloc(sizeof(ucontext_t));
        //    memset(ctx, 0, sizeof(ucontext_t));
        //    getcontext(ctx);
        //    ctx->uc_stack.ss_sp = malloc(stack);
        //    ctx->uc_stack.ss_size = stack;
        //    ctx->uc_stack.ss_flags = 0;
        //#if defined FBSEPCMP
        //do {
        //      uintptr_t fst = (uintptr_t)ret;
        //      uintptr_t scd = (uintptr_t)ret;
        //      fst &= 0x00000000ffffffff;
        //      scd >>= 32;
        //      makecontext(ctx, (void(*)())fiber_proc_impl, 2, (unsigned)fst, (unsigned)scd);
        //} while (0);
        //#else /* FBSEPCMP */
        //    makecontext(ctx, (void(*)())fiber_proc_impl, 1, ret);
        //#endif /* FBSEPCMP */
        //    ret->current = primary->current;
        //    ret->stack_size = stack;
        //    ret->context = ctx;
        //} else {
        //    ret->current = (fiber_t**)malloc(sizeof(fiber_t*));
        //    ctx = (ucontext_t*)malloc(sizeof(ucontext_t));
        //    memset(ctx, 0, sizeof(ucontext_t));
        //    *ret->current = ret;
        //    ret->stack_size = 0;
        //    ret->context = ctx;
        //}
        //return ret;
};

