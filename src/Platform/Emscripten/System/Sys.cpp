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
/*
** For the latest info, see https://github.com/paladin-t/fiber/
**
** Copyright (C) 2017 - 2018 Wang Renxin
**
** Just #include "fiber.h" before using this library.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of
** this software and associated documentation files (the "Software"), to deal in
** the Software without restriction, including without limitation the rights to
** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
** the Software, and to permit persons to whom the Software is furnished to do so,
** subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
** FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
** COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
** IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Sys.h"
#include <iostream>
#include <memory.h>
#include <stdint.h>
#include <stdlib.h>
#include <emscripten.h>
#include <sys/eventfd.h>
#include <bits/syscall.h>
#include <sys/timerfd.h>
//#include <sys/epoll.h>

#include <emscripten.h>

extern "C" {
#include <sys/timerfd.h>
#include "syscall.h"
}

typedef struct fiber_t {
	/**< Current fiber, all fibers within the same thread should share the same "current" pointer. */
	struct fiber_t** current;
	/**< How much memory is reserved for this fiber. */
	size_t stack_size;
	/**< Raw context of this fiber. */
	void* context;
	/**< Processer of this fiber, as callback. */
	fiber_proc proc;
	/**< Fiber carries some extra data with `userdata`. */
	void* userdata;
} fiber_t;

typedef struct emscripten_context_t {
        fiber_t* primary;
        union {
                emscripten_coroutine emco;
                fiber_t* next;
        };
} emscripten_context_t;

Sys::Sys() {
}

Sys::~Sys() {
}

// SYS_epoll_create1 at 329
int Sys::epoll_create1(int flags)
{
  int retval = EM_ASM_INT({
    err('warning: fedoragold syscall: epoll_create1');
    emscripten_syscall(SYS_epoll_create1, $0);
    //return _syscall(SYS_epoll_create1, $0);
  }, flags);

  return retval;
}

// SYS_epoll_ctl at 255
int Sys::epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
  int retval = EM_ASM_INT({
    err('warning: fedoragold syscall: epoll_ctl');
    emscripten_syscall(SYS_epoll_ctl, $0, $1, $2, $3);
    //return _syscall(SYS_epoll_ctl, $0, $1, $2, $3);
  }, epfd, op, fd, event);

  return retval;
}

// SYS_epoll_wait at 256
int Sys::epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
  int retval = EM_ASM_INT({
    err('warning: fedoragold syscall: epoll_wait');
    emscripten_syscall(SYS_epoll_wait, $0, $1, $2, $3);
    //return _syscall(SYS_epoll_wait, $0, $1, $2, $3);
  }, epfd, events, maxevents, timeout);

  return retval;
}

// SYS_timerfd_create at 322
int Sys::timerfd_create(int clockid, int flags)
{
  int retval = EM_ASM_INT({
    err('warning: fedoragold syscall: timerfd_create');
    return emscripten_syscall(SYS_timerfd_create, $0, $1);
    //return _syscall(SYS_timerfd_create, $0, $1);
  }, clockid, flags);

  return retval;
}

// SYS_timerfd_settime at 325
int Sys::timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *old_value)
{ 
  int retval = EM_ASM_INT({
    err('warning: fedoragold syscall: timerfd_settime');
    return emscripten_syscall(SYS_timerfd_settime, $0, $1, $2, $3);
    //return _syscall(SYS_timerfd_settime, $0, $1, $2, $3);
  }, fd, flags, new_value, old_value);

  return retval;
}

// SYS_eventfd at 323
int Sys::eventfd(unsigned int count, int flags)
{
  int retval = EM_ASM_INT({
    err('warning: fedoragold syscall: eventfd');
    return emscripten_syscall(SYS_eventfd, $0, $1);
    //return _syscall(SYS_eventfd, $0, $1);
  }, count, flags);

  return retval;
}

void Sys::makecontext(void *context, size_t stack, void (*func)(), int argc, int *argv)
{
  fiber_t* fiber = fiber_create((fiber_t*)context, stack, (fiber_proc)func, (void*)argv); 
}

void Sys::deletecontext(void *context)
{
  fiber_delete((fiber_t*)context);
}

int Sys::getcontext(void *ucp)
{
  const fiber_t* const fb = (fiber_t*)ucp;
  ucp = (void *)fb->current;
  return 0;
}

int Sys::swapcontext(void *oucp, const void *ucp)
{
  // jojapoppa, do I need to set oucp inside the fiber_t struct?

  return (int)fiber_switch((fiber_t*)ucp);
}

void fiber_proc_impl(fiber_t* fb) {
	if (!fb) return;

	fb->proc(fb);
}

bool_t Sys::fiber_is_current(const fiber_t* const fb) {
	if (!fb) return false;

	return *fb->current == fb;
}

fiber_t* Sys::fiber_create(fiber_t* primary, size_t stack, fiber_proc run, void* userdata) {
	fiber_t* ret = 0;
	emscripten_context_t* ctx = 0;
	if ((!primary && run) || (primary && !run)) return ret;
	ret = (fiber_t*)malloc(sizeof(fiber_t));
	ret->proc = run;
	ret->userdata = userdata;
	if (primary && run) {
		if (!stack) stack = FIBER_STACK_SIZE;
		ret->current = primary->current;
		ret->stack_size = stack;
		ctx = (emscripten_context_t*)malloc(sizeof(emscripten_context_t));
		memset(ctx, 0, sizeof(emscripten_context_t));
		ctx->primary = primary;
		ctx->emco = emscripten_coroutine_create((em_arg_callback_func)fiber_proc_impl, ret, stack);
		ret->context = ctx;
	} else {
		ret->current = (fiber_t**)malloc(sizeof(fiber_t*));
		*ret->current = ret;
		ret->stack_size = 0;
		ctx = (emscripten_context_t*)malloc(sizeof(emscripten_context_t));
		memset(ctx, 0, sizeof(emscripten_context_t));
		ctx->primary = ret;
		ret->context = ctx;
	}

	return ret;
}

bool_t Sys::fiber_delete(fiber_t* fb) {
	emscripten_context_t* ctx = 0;
	if (!fb) return false;
	ctx = (emscripten_context_t*)fb->context;
	if (fb->proc) {
		// Using `free` to clean up the emscripten coroutine, this is an undocumented usage,
		// see https://github.com/kripken/emscripten/issues/6540 for discussion.
		free(ctx->emco);
		free(ctx);
	} else {
		free(ctx);
		free(fb->current);
	}
	free(fb);

	return true;
}

bool_t Sys::fiber_switch(fiber_t* fb) {
	fiber_t* prev = 0;
	fiber_t* primary = 0;
	emscripten_context_t* ctx = 0;
	if (!fb) return false;
	prev = *fb->current;
	*fb->current = fb;
	primary = (fiber_t*)((emscripten_context_t*)fb->context)->primary;
	ctx = (emscripten_context_t*)primary->context;
	ctx->next = fb;
	if (prev->proc) {
		emscripten_yield();
	} else {
		while (ctx->next && ctx->next != primary) {
			emscripten_context_t* to = (emscripten_context_t*)ctx->next->context;
			*fb->current = ctx->next;
			emscripten_coroutine_next(to->emco);
		}
	}

	return true;
}
